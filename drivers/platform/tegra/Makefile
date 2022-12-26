# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia-oot)

obj-m += firmwares-class.o
obj-m += firmwares-inventory.o

tegra-bootloader-debug-objs := tegra_bootloader_debug.o
tegra-bootloader-debug-objs += tegra_bootloader_debug_init.o
obj-m += tegra-bootloader-debug.o

tegra-cactmon-objs := cactmon.o
tegra-cactmon-objs += actmon_common.o
obj-m += tegra-cactmon.o
obj-m += tegra-fsicom.o

tegra-camera-rtcpu-objs :=  tegra-camera-rtcpu-base.o rtcpu/tegra-rtcpu-trace.o rtcpu/rtcpu-monitor.o
obj-m += tegra-camera-rtcpu.o

obj-m += cvnas/
obj-m += hwpm/
obj-m += mce/
obj-m += uncore_pmu/
obj-m += mc-hwpm.o
obj-m += mc-utils/
obj-m += dce/
obj-m += rtcpu/
