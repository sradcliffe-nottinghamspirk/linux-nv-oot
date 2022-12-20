# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

#
# Makefile for NvSciIpc driver
#

ifeq ($(CONFIG_TEGRA_OOT_MODULE),m)
ccflags-y += -DCONFIG_TEGRA_OOT_MODULE
ifeq ($(CONFIG_TEGRA_VIRTUALIZATION),y)
ccflags-y += -DCONFIG_TEGRA_VIRTUALIZATION
endif
endif
obj-m += nvsciipc.o

