# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

GCOV_PROFILE := y

ifdef CONFIG_TEGRA_GRHOST
ccflags-y += -DCONFIG_TEGRA_NVDLA_CHANNEL
endif

# Enable FUSE UPSTREAM
CONFIG_TEGRA_FUSE_UPSTREAM := y

ccflags-y += -Werror
ccflags-y += -DCONFIG_TEGRA_HOST1X
ccflags-y += -DCONFIG_TEGRA_FUSE_UPSTREAM

nvhost-nvdla-objs  = \
		nvdla.o \
		nvdla_buffer.o \
		nvdla_ioctl.o \
		dla_queue.o \
		nvdla_queue.o \
		nvdla_debug.o

ifdef CONFIG_TEGRA_GRHOST
nvhost-nvdla-objs += dla_channel.o
endif

obj-m += nvhost-nvdla.o
