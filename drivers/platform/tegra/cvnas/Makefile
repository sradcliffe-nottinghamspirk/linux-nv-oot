# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.
#

ccflags-y += -Werror

# Enable UPSTREM FUSE as default for OOT module
CONFIG_TEGRA_FUSE_UPSTREAM := y
ccflags-y += -DCONFIG_TEGRA_FUSE_UPSTREAM

obj-m += cvnas.o
ccflags-y += -DCVNAS_MODULE
