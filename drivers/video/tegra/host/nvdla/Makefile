# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

GCOV_PROFILE := y

ifdef CONFIG_TEGRA_GRHOST
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -DCONFIG_TEGRA_NVDLA_CHANNEL
endif

ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -I$(srctree.nvidia)/include/uapi/linux
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host/nvdla
ccflags-y += -Werror

# When using the upstream host1x driver, the Makefile must define the
# srctree.host1x path in order to find the necessary header files for
# the upstream host1x driver.
ccflags-y += -I$(srctree.host1x)/include
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
