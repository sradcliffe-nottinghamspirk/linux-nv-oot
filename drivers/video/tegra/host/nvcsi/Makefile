# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia-oot)/drivers/media/platform/tegra

nvhost-nvcsi-objs  = \
		nvcsi.o \
		deskew.o

obj-m += nvhost-nvcsi.o

nvhost-nvcsi-t194-objs = nvcsi-t194.o
obj-m += nvhost-nvcsi-t194.o
