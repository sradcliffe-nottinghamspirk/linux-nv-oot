/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 */

#ifndef __UAPI_ISC_DEV_H__
#define __UAPI_ISC_DEV_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#define ISC_DEV_PKG_FLAG_WR	1

#define ISC_DEV_IOCTL_RW	_IOW('o', 1, struct isc_dev_package)

struct __attribute__ ((__packed__)) isc_dev_package {
	__u16 offset;
	__u16 offset_len;
	__u32 size;
	__u32 flags;
	unsigned long buffer;
};

#ifdef __KERNEL__
#ifdef CONFIG_COMPAT
#define ISC_DEV_IOCTL_RW32	_IOW('o', 1, struct isc_dev_package32)

struct __attribute__ ((__packed__)) isc_dev_package32 {
	__u16 offset;
	__u16 offset_len;
	__u32 size;
	__u32 flags;
	__u32 buffer;
};
#endif /* CONFIG_COMPAT */
#endif /* __KERNEL__ */

#endif  /* __UAPI_ISC_DEV_H__ */
