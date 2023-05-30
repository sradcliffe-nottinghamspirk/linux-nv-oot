# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

tegra-bootloader-debug-objs := tegra_bootloader_debug.o
tegra-bootloader-debug-objs += tegra_bootloader_debug_init.o
obj-m += tegra-bootloader-debug.o

tegra-cactmon-objs := cactmon.o
tegra-cactmon-objs += actmon_common.o
obj-m += tegra-cactmon.o
obj-m += tegra-cactmon-mc-all.o
obj-m += tegra-fsicom.o

obj-m += mce/
ifdef CONFIG_PERF_EVENTS
obj-m += uncore_pmu/
endif
obj-m += mc-hwpm.o
obj-m += mc-utils/
obj-m += dce/
obj-m += psc/
obj-m += rtcpu/
obj-m += aon/
obj-m += tegra-uss-io-proxy.o
