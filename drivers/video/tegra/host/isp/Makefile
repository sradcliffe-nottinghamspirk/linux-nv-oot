# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia)/drivers/gpu/host1x/hw/
LINUXINCLUDE += -I$(srctree.nvidia)/drivers/gpu/host1x/include
LINUXINCLUDE += -I$(srctree.nvidia)/include
LINUXINCLUDE += -I$(srctree.nvidia-oot)/drivers/video/tegra/host
LINUXINCLUDE += -I$(srctree.nvidia-oot)/include
LINUXINCLUDE += -DCONFIG_TEGRA_HOST1X

nvhost-isp5-objs = isp5.o
obj-m += nvhost-isp5.o
