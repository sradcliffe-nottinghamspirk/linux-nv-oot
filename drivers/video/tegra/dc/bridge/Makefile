# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.


# Set config when build as OOT module.
ifeq ($(CONFIG_TEGRA_OOT_MODULE),m)
CONFIG_MAXIM_GMSL_DP_SERIALIZER := m
CONFIG_TI_FPDLINK_DP_SERIALIZER := m
endif

obj-$(CONFIG_MAXIM_GMSL_DP_SERIALIZER) += maxim_gmsl_dp_serializer.o
obj-$(CONFIG_TI_FPDLINK_DP_SERIALIZER) += ti_fpdlink_dp_serializer.o
