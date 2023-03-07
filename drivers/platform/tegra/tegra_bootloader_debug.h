// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.

#ifndef __TEGRA_BOOTLOADER_DEBUG_H
#define __TEGRA_BOOTLOADER_DEBUG_H
extern phys_addr_t tegra_bl_debug_data_start;
extern phys_addr_t tegra_bl_debug_data_size;
extern phys_addr_t tegra_bl_prof_start;
extern phys_addr_t tegra_bl_prof_size;
extern phys_addr_t tegra_bl_prof_ro_start;
extern phys_addr_t tegra_bl_prof_ro_size;
extern phys_addr_t tegra_bl_bcp_start;
extern phys_addr_t tegra_bl_bcp_size;

size_t tegra_bl_add_profiler_entry(const char *buf, size_t len);

#endif
