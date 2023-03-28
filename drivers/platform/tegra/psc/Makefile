# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2020-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

GCOV_PROFILE := y
CONFIG_TEGRA_PSC := m

ccflags-y += -I$(srctree.nvidia)/drivers/platform/tegra/include

ccflags-y += -Werror

obj-$(CONFIG_TEGRA_PSC) += tegra23x_psc.o
tegra23x_psc-objs := tegra23x_psc_mailbox.o

tegra23x_psc-$(CONFIG_DEBUG_FS) += psc_debug.o
