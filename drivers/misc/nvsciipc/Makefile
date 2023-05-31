# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

#
# Makefile for NvSciIpc driver
#

ccflags-y += -I$(PWD)

ifeq ($(CONFIG_TEGRA_VIRTUALIZATION),y)
ccflags-y += -DCONFIG_TEGRA_VIRTUALIZATION
endif

obj-m += nvsciipc.o