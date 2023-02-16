# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES.  All rights reserved.

obj-m += ufs-tegra.o
ifeq ($(CONFIG_TEGRA_UFS_PROVISIONING),y)
ufs-tegra-objs += ufs-provision.o
endif
