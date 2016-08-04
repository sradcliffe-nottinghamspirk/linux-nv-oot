/*
 * NVDLA IOCTL for T194
 *
 * Copyright (c) 2016, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>

#include "dev.h"
#include "bus_client.h"
#include "nvhost_acm.h"
#include "nvhost_buffer.h"
#include "flcn/flcn.h"
#include "flcn/hw_flcn.h"

#include "t194/t194.h"
#include "nvhost_queue.h"

#include "nvdla/nvdla.h"
#include "nvhost_nvdla_ioctl.h"
#include "dla_os_interface.h"

#define DEBUG_BUFFER_SIZE 0x100
#define FLCN_IDLE_TIMEOUT_DEFAULT	10000	/* 10 milliseconds */
#define ALIGNED_DMA(x) ((x >> 8) & 0xffffffff)

static DEFINE_DMA_ATTRS(attrs);

struct nvdla_private {
	struct platform_device *pdev;
	struct nvhost_queue *queue;
	struct nvhost_buffers *buffers;
};

static int nvdla_ctrl_pin(struct nvdla_private *priv, void *arg)
{
	u32 *handles;
	int err = 0;
	struct nvdla_ctrl_pin_unpin_args *buf_list =
			(struct nvdla_ctrl_pin_unpin_args *)arg;
	u32 count = buf_list->num_buffers;

	handles = kcalloc(count, sizeof(u32), GFP_KERNEL);
	if (!handles)
		return -ENOMEM;

	if (copy_from_user(handles, (void __user *)buf_list->buffers,
			(count * sizeof(u32)))) {
		err = -EFAULT;
		goto nvdla_buffer_cpy_err;
	}

	err = nvhost_buffer_pin(priv->buffers, handles, count);

nvdla_buffer_cpy_err:
	kfree(handles);
	return err;
}

static int nvdla_ctrl_unpin(struct nvdla_private *priv, void *arg)
{
	u32 *handles;
	int err = 0;
	struct nvdla_ctrl_pin_unpin_args *buf_list =
			(struct nvdla_ctrl_pin_unpin_args *)arg;
	u32 count = buf_list->num_buffers;

	handles = kcalloc(count, sizeof(u32), GFP_KERNEL);
	if (!handles)
		return -ENOMEM;

	if (copy_from_user(handles, (void __user *)buf_list->buffers,
		(count * sizeof(u32)))) {
		err = -EFAULT;
		goto nvdla_buffer_cpy_err;
	}

	nvhost_buffer_unpin(priv->buffers, handles, count);

nvdla_buffer_cpy_err:
	kfree(handles);
	return err;
}

static int nvdla_ctrl_ping(struct platform_device *pdev,
			   struct nvdla_ctrl_ping_args *args)
{
	DEFINE_DMA_ATTRS(ping_attrs);
	dma_addr_t ping_pa;
	u32 *ping_va;

	u32 timeout = FLCN_IDLE_TIMEOUT_DEFAULT * 5;
	int err = 0;

	/* make sure that device is powered on */
	nvhost_module_busy(pdev);

	/* allocate ping buffer */
	ping_va = dma_alloc_attrs(&pdev->dev,
				  DEBUG_BUFFER_SIZE, &ping_pa,
				  GFP_KERNEL, &ping_attrs);
	if (!ping_va) {
		dev_err(&pdev->dev, "dma memory allocation failed for ping");
		err = -ENOMEM;
		goto fail_to_alloc;
	}

	/* pass ping value to falcon */
	*ping_va = args->in_challenge;

	/* run ping cmd */
	nvdla_send_cmd(pdev, DLA_CMD_PING, ALIGNED_DMA(ping_pa));

	/* wait for falcon to idle */
	err = flcn_wait_idle(pdev, &timeout);
	if (err != 0) {
		dev_err(&pdev->dev, "failed for wait for idle in timeout");
		goto fail_to_idle;
	}

	/* out value should have (in_challenge * 4) */
	args->out_response = *ping_va;

	if (args->out_response != args->in_challenge*4) {
		dev_err(&pdev->dev, "ping cmd failed. Falcon is not active");
		err = -EINVAL;
	}

fail_to_idle:
	if (ping_va)
		dma_free_attrs(&pdev->dev, DEBUG_BUFFER_SIZE,
			       ping_va, ping_pa, &attrs);
fail_to_alloc:
	nvhost_module_idle(pdev);

	return err;
}

static long nvdla_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	struct nvdla_private *priv = file->private_data;
	struct platform_device *pdev = priv->pdev;
	u8 buf[NVDLA_IOCTL_CTRL_MAX_ARG_SIZE] __aligned(sizeof(u64));
	int err = 0;

	/* check for valid IOCTL cmd */
	if ((_IOC_TYPE(cmd) != NVHOST_NVDLA_IOCTL_MAGIC) ||
	    (_IOC_NR(cmd) == 0) ||
	    (_IOC_NR(cmd) > NVDLA_IOCTL_CTRL_LAST) ||
	    (_IOC_SIZE(cmd) > NVDLA_IOCTL_CTRL_MAX_ARG_SIZE)) {
		return -ENOIOCTLCMD;
	}

	nvhost_dbg_fn("%s: pdev:%p priv:%p\n", __func__, pdev, priv);

	/* copy from user for read commands */
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		if (copy_from_user(buf, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;

	/* handle IOCTL cmd */
	switch (cmd) {
	case NVDLA_IOCTL_CTRL_PING:
		err = nvdla_ctrl_ping(pdev, (void *)buf);
		break;
	case NVDLA_IOCTL_CTRL_PIN:
		err = nvdla_ctrl_pin(priv, (void *)buf);
		break;
	case NVDLA_IOCTL_CTRL_UNPIN:
		err = nvdla_ctrl_unpin(priv, (void *)buf);
		break;
	default:
		err = -ENOIOCTLCMD;
		break;
	}

	/* copy to user for write commands */
	if ((err == 0) && (_IOC_DIR(cmd) & _IOC_READ))
		err = copy_to_user((void __user *)arg, buf, _IOC_SIZE(cmd));

	return err;
}

static int nvdla_open(struct inode *inode, struct file *file)
{
	struct nvhost_device_data *pdata = container_of(inode->i_cdev,
					struct nvhost_device_data, ctrl_cdev);
	struct platform_device *pdev = pdata->pdev;
	struct nvdla_device *nvdla_dev = pdata->private_data;
	struct nvdla_private *priv;
	int err = 0;

	priv = kmalloc(sizeof(*priv), GFP_KERNEL);
	if (unlikely(priv == NULL)) {
		err = -ENOMEM;
		goto err_alloc_priv;
	}

	file->private_data = priv;
	priv->pdev = pdev;

	nvhost_dbg_fn("%s: pdev:%p priv:%p\n", __func__, pdev, priv);

	/* add priv to client list */
	err = nvhost_module_add_client(pdev, priv);
	if (err < 0)
		goto err_add_client;

	priv->buffers = nvhost_buffer_init(pdev);
	if (IS_ERR(priv->buffers)) {
		err = PTR_ERR(priv->buffers);
		goto err_alloc_buffer;
	}

	priv->queue = nvhost_queue_alloc(nvdla_dev->pool);
	if (IS_ERR(priv->queue)) {
		err = PTR_ERR(priv->queue);
		goto err_alloc_queue;
	}

	return nonseekable_open(inode, file);

err_alloc_queue:
	nvhost_module_remove_client(pdev, priv);
err_alloc_buffer:
	kfree(priv->buffers);
err_add_client:
	kfree(priv);
err_alloc_priv:
	return err;
}

static int nvdla_release(struct inode *inode, struct file *file)
{
	struct nvdla_private *priv = file->private_data;
	struct platform_device *pdev = priv->pdev;

	nvhost_dbg_fn("%s: pdev:%p priv:%p\n", __func__, pdev, priv);

	nvhost_queue_abort(priv->queue);
	nvhost_queue_put(priv->queue);
	nvhost_buffer_put(priv->buffers);
	nvhost_module_remove_client(pdev, priv);

	kfree(priv);
	return 0;
}

const struct file_operations tegra_nvdla_ctrl_ops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.unlocked_ioctl = nvdla_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = nvdla_ioctl,
#endif
	.open = nvdla_open,
	.release = nvdla_release,
};
