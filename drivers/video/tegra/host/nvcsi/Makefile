# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia)/drivers/gpu/host1x/hw/
LINUXINCLUDE += -I$(srctree.nvidia)/drivers/gpu/host1x/include
LINUXINCLUDE += -I$(srctree.nvidia)/include
LINUXINCLUDE += -I$(srctree.nvidia-oot)/drivers/media/platform/tegra

nvhost-nvcsi-objs  = \
		nvcsi.o \
		deskew.o

obj-m += nvhost-nvcsi.o

nvhost-nvcsi-t194-objs = nvcsi-t194.o
obj-m += nvhost-nvcsi-t194.o
