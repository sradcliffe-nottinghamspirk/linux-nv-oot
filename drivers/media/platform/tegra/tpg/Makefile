# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.

ccflags-y += -Werror

LINUXINCLUDE += -I$(srctree.nvidia)/drivers/gpu/host1x/hw/
LINUXINCLUDE += -I$(srctree.nvidia)/drivers/video/tegra/host/

nvhost-vi-tpg-t19x-objs += tpg_t19x.o
obj-m += nvhost-vi-tpg-t19x.o
