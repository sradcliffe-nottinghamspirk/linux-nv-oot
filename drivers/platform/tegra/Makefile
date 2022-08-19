# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

obj-m += bad.o

obj-m += firmwares-class.o
obj-m += firmwares-inventory.o

tegra-bootloader-debug-objs := tegra_bootloader_debug.o
tegra-bootloader-debug-objs += tegra_bootloader_debug_init.o
obj-m += tegra-bootloader-debug.o

tegra-cactmon-objs := cactmon.o
tegra-cactmon-objs += actmon_common.o
obj-m += tegra-cactmon.o
obj-m += tegra-epl.o
obj-m += tegra-fsicom.o

obj-m += cvnas/
obj-m += hwpm/
obj-m += mce/
obj-m += uncore_pmu/
