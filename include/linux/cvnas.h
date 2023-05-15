/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2023, NVIDIA Corporation. All rights reserved.
 */

#ifndef __LINUX_CVNAS_H
#define __LINUX_CVNAS_H

#include <linux/types.h>

#if defined(CONFIG_TEGRA_CVNAS) || defined(CVNAS_MODULE)
int nvcvnas_busy(void);
int nvcvnas_busy_no_rpm(void);
int nvcvnas_idle(void);
int nvcvnas_idle_no_rpm(void);
int is_nvcvnas_probed(void);
phys_addr_t nvcvnas_get_cvsram_base(void);
size_t nvcvnas_get_cvsram_size(void);
int is_nvcvnas_clk_enabled(void);
#else
static inline int nvcvnas_busy(void) { return 0; }
static inline int nvcvnas_busy_no_rpm(void) { return 0; }
static inline int nvcvnas_idle(void) { return 0; }
static inline int nvcvnas_idle_no_rpm(void) { return 0; }
static inline int is_nvcvnas_probed(void) { return 0; }
static inline phys_addr_t nvcvnas_get_cvsram_base(void) { return 0; }
static inline size_t nvcvnas_get_cvsram_size(void) { return 0; }
static inline int is_nvcvnas_clk_enabled(void) { return 0; }
#endif /* CONFIG_TEGRA_CVNAS || CVNAS_MODULE */

#endif /* __LINUX_CVNAS_H */
