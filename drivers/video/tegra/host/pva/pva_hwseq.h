/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
 */

#ifndef PVA_HWSEQ_H
#define PVA_HWSEQ_H

#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>

#define PVA_HWSEQ_FRAME_ADDR	0xC0DE
#define PVA_HWSEQ_DESC_ADDR	0xDEAD

struct pva_hwseq_frame_header_s {
	u16	fid;
	u8	fr;
	u8	no_cr;
	u16	to;
	u16	fo;
	u8	pad_r;
	u8	pad_t;
	u8	pad_l;
	u8	pad_b;
} __packed;

struct pva_hwseq_cr_header_s {
	u8	dec;
	u8	crr;
	u16	cro;
} __packed;

struct pva_hwseq_desc_header_s {
	u8	did1;
	u8	dr1;
	u8	did2;
	u8	dr2;
} __packed;

struct pva_hw_sweq_blob_s {
	struct pva_hwseq_frame_header_s f_header;
	struct pva_hwseq_cr_header_s cr_header;
	struct pva_hwseq_desc_header_s desc_header;
} __packed;

static inline bool is_frame_mode(u16 id)
{
	return (id == PVA_HWSEQ_FRAME_ADDR);
}

static inline bool is_desc_mode(u16 id)
{
	return (id == PVA_HWSEQ_DESC_ADDR);
}
#endif

