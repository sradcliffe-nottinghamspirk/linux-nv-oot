/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 */

#include <linux/module.h>
#include <soc/tegra/ivc_ext.h>

#define TEGRA_IVC_ALIGN 64

/*
 * IVC channel reset protocol.
 *
 * Each end uses its tx_channel.state to indicate its synchronization state.
 */
enum tegra_ivc_state {
	/*
	 * This value is zero for backwards compatibility with services that
	 * assume channels to be initially zeroed. Such channels are in an
	 * initially valid state, but cannot be asynchronously reset, and must
	 * maintain a valid state at all times.
	 *
	 * The transmitting end can enter the established state from the sync or
	 * ack state when it observes the receiving endpoint in the ack or
	 * established state, indicating that has cleared the counters in our
	 * rx_channel.
	 */
	TEGRA_IVC_STATE_ESTABLISHED = 0,

	/*
	 * If an endpoint is observed in the sync state, the remote endpoint is
	 * allowed to clear the counters it owns asynchronously with respect to
	 * the current endpoint. Therefore, the current endpoint is no longer
	 * allowed to communicate.
	 */
	TEGRA_IVC_STATE_SYNC,

	/*
	 * When the transmitting end observes the receiving end in the sync
	 * state, it can clear the w_count and r_count and transition to the ack
	 * state. If the remote endpoint observes us in the ack state, it can
	 * return to the established state once it has cleared its counters.
	 */
	TEGRA_IVC_STATE_ACK
};

/*
 * This structure is divided into two-cache aligned parts, the first is only
 * written through the tx.channel pointer, while the second is only written
 * through the rx.channel pointer. This delineates ownership of the cache
 * lines, which is critical to performance and necessary in non-cache coherent
 * implementations.
 */
struct tegra_ivc_header {
	union {
		struct {
			/* fields owned by the transmitting end */
			u32 count;
			u32 state;
		};

		u8 pad[TEGRA_IVC_ALIGN];
	} tx;

	union {
		/* fields owned by the receiving end */
		u32 count;
		u8 pad[TEGRA_IVC_ALIGN];
	} rx;
};

int tegra_ivc_channel_notified(struct tegra_ivc *ivc)
{
	return tegra_ivc_notified(ivc);
}
EXPORT_SYMBOL(tegra_ivc_channel_notified);

void tegra_ivc_channel_reset(struct tegra_ivc *ivc)
{
	tegra_ivc_reset(ivc);
}
EXPORT_SYMBOL(tegra_ivc_channel_reset);

static inline void tegra_ivc_invalidate(struct tegra_ivc *ivc, dma_addr_t phys)
{
	if (!ivc->peer)
		return;

	dma_sync_single_for_cpu(ivc->peer, phys, TEGRA_IVC_ALIGN,
				DMA_FROM_DEVICE);
}

bool tegra_ivc_empty(struct tegra_ivc *ivc,
				   struct tegra_ivc_header *header)
{
	/*
	 * This function performs multiple checks on the same values with
	 * security implications, so create snapshots with READ_ONCE() to
	 * ensure that these checks use the same values.
	 */
	u32 tx = READ_ONCE(header->tx.count);
	u32 rx = READ_ONCE(header->rx.count);

	/*
	 * Perform an over-full check to prevent denial of service attacks
	 * where a server could be easily fooled into believing that there's
	 * an extremely large number of frames ready, since receivers are not
	 * expected to check for full or over-full conditions.
	 *
	 * Although the channel isn't empty, this is an invalid case caused by
	 * a potentially malicious peer, so returning empty is safer, because
	 * it gives the impression that the channel has gone silent.
	 */
	if (tx - rx > ivc->num_frames)
		return true;

	return tx == rx;
}
EXPORT_SYMBOL(tegra_ivc_empty);

static inline bool tegra_ivc_full(struct tegra_ivc *ivc,
				  struct tegra_ivc_header *header)
{
	u32 tx = READ_ONCE(header->tx.count);
	u32 rx = READ_ONCE(header->rx.count);

	/*
	 * Invalid cases where the counters indicate that the queue is over
	 * capacity also appear full.
	 */
	return tx - rx >= ivc->num_frames;
}

static inline int tegra_ivc_check_read(struct tegra_ivc *ivc)
{
	unsigned int offset = offsetof(struct tegra_ivc_header, tx.count);

	/*
	 * tx.channel->state is set locally, so it is not synchronized with
	 * state from the remote peer. The remote peer cannot reset its
	 * transmit counters until we've acknowledged its synchronization
	 * request, so no additional synchronization is required because an
	 * asynchronous transition of rx.channel->state to
	 * TEGRA_IVC_STATE_ACK is not allowed.
	 */
	if (ivc->tx.channel->tx.state != TEGRA_IVC_STATE_ESTABLISHED)
		return -ECONNRESET;

	/*
	 * Avoid unnecessary invalidations when performing repeated accesses
	 * to an IVC channel by checking the old queue pointers first.
	 *
	 * Synchronization is only necessary when these pointers indicate
	 * empty or full.
	 */
	if (!tegra_ivc_empty(ivc, ivc->rx.channel))
		return 0;

	tegra_ivc_invalidate(ivc, ivc->rx.phys + offset);

	if (tegra_ivc_empty(ivc, ivc->rx.channel))
		return -ENOSPC;

	return 0;
}

static inline int tegra_ivc_check_write(struct tegra_ivc *ivc)
{
	unsigned int offset = offsetof(struct tegra_ivc_header, rx.count);

	if (ivc->tx.channel->tx.state != TEGRA_IVC_STATE_ESTABLISHED)
		return -ECONNRESET;

	if (!tegra_ivc_full(ivc, ivc->tx.channel))
		return 0;

	tegra_ivc_invalidate(ivc, ivc->tx.phys + offset);

	if (tegra_ivc_full(ivc, ivc->tx.channel))
		return -ENOSPC;

	return 0;
}

int tegra_ivc_can_read(struct tegra_ivc *ivc)
{
	return tegra_ivc_check_read(ivc) == 0;
}
EXPORT_SYMBOL(tegra_ivc_can_read);

int tegra_ivc_can_write(struct tegra_ivc *ivc)
{
	return tegra_ivc_check_write(ivc) == 0;
}
EXPORT_SYMBOL(tegra_ivc_can_write);

int tegra_ivc_read(struct tegra_ivc *ivc, void __user *usr_buf, void *buf, size_t max_read)
{
	void *frame;

	BUG_ON(buf && usr_buf);

	/* get next frame to be read from IVC channel */
	frame = tegra_ivc_read_get_next_frame(ivc);
	if (IS_ERR(frame)) {
		return PTR_ERR(frame);
	}

	/* update the buffer with read data*/
	if (buf) {
		memcpy(buf, frame, max_read);
	} else if (usr_buf) {
		if (copy_to_user(usr_buf, frame, max_read))
			return -EFAULT;
	} else
		BUG();

	/* Advance to next read frame*/
	if (tegra_ivc_read_advance(ivc) == 0)
		return max_read;
	else
		return 0;
}
EXPORT_SYMBOL(tegra_ivc_read);

int tegra_ivc_read_peek(struct tegra_ivc *ivc, void __user *usr_buf, void *buf, size_t offset, size_t size)
{
	void *frame;

	BUG_ON(buf && usr_buf);

	/* get next frame to be read from IVC channel */
	frame = tegra_ivc_read_get_next_frame(ivc);
	if (IS_ERR(frame)) {
		return PTR_ERR(frame);
	}

	/* update the buffer with read data*/
	if (buf) {
		memcpy(buf, frame + offset, size);
	} else if (usr_buf) {
		if (copy_to_user(usr_buf, frame + offset, size))
			return -EFAULT;
	} else
		BUG();

	return size;
}
EXPORT_SYMBOL(tegra_ivc_read_peek);

int tegra_ivc_write(struct tegra_ivc *ivc, const void __user *usr_buf, const void *buf, size_t size)
{
	void *frame;

	BUG_ON(buf && usr_buf);

	/* get next frame to be written from IVC channel */
	frame = tegra_ivc_write_get_next_frame(ivc);
	if (IS_ERR(frame)) {
		return PTR_ERR(frame);
	}

	/* update the write frame with data buffer*/
	if (buf) {
		memcpy(frame, buf, size);
	} else if (usr_buf) {
		if (copy_from_user(frame, usr_buf, size))
			return -EFAULT;
	} else
		BUG();

	/* Advance to next write frame*/
	if (tegra_ivc_write_advance(ivc) == 0)
		return size;
	else
		return 0;
}
EXPORT_SYMBOL(tegra_ivc_write);

int tegra_ivc_channel_sync(struct tegra_ivc *ivc)
{
	if ((ivc == NULL) || (ivc->num_frames == 0)) {
		return -EINVAL;
	} else {
		ivc->tx.position = ivc->tx.channel->tx.count % ivc->num_frames;
		ivc->rx.position = ivc->rx.channel->rx.count % ivc->num_frames;
	}
	return 0;
}
EXPORT_SYMBOL(tegra_ivc_channel_sync);

static inline u32 tegra_ivc_available(struct tegra_ivc *ivc,
				      struct tegra_ivc_header *header)
{
	u32 tx = READ_ONCE(header->tx.count);
	u32 rx = READ_ONCE(header->rx.count);

	/*
	 * This function isn't expected to be used in scenarios where an
	 * over-full situation can lead to denial of service attacks. See the
	 * comment in tegra_ivc_empty() for an explanation about special
	 * over-full considerations.
	 */
	return tx - rx;
}

uint32_t tegra_ivc_frames_available(struct tegra_ivc *ivc, struct tegra_ivc_header *header)
{
	return (ivc->num_frames - tegra_ivc_available(ivc, header));
}
EXPORT_SYMBOL(tegra_ivc_frames_available);

/* Inserting this driver as module to export
 * extended IVC driver APIs
 */
static int __init ivc_driver_init(void)
{
	pr_info("Inserting ivc_ext.ko module");
	return 0;
}
module_init(ivc_driver_init);

static void __exit ivc_driver_exit(void)
{
	pr_info("Removing ivc_ext.ko module");
}
module_exit(ivc_driver_exit);

MODULE_AUTHOR("Manish Bhardwaj <mbhardwaj@nvidia.com>");
MODULE_DESCRIPTION("Extended IVC Driver");
MODULE_LICENSE("GPL v2");
