# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.

ifdef CONFIG_TEGRA_HOST1X
obj-m += tegra-se-nvhost.o
obj-m += tegra-hv-vse-safety.o
endif
obj-m += tegra-se-nvrng.o

ifdef CONFIG_TEGRA_HOST1X
ifneq ($(CONFIG_TEGRA_SYSTEM_TYPE_ACK),y)
ccflags-y += -Wframe-larger-than=2560
endif
obj-m += tegra-nvvse-cryptodev.o
endif
