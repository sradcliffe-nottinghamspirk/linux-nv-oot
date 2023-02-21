# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

#
# Makefile for Extended IVC Driver and BPMP driver
#

LINUX_VERSION := $(shell expr $(VERSION) \* 256 + $(PATCHLEVEL))
LINUX_VERSION_6_2 := $(shell expr 6 \* 256 + 2)

# ivc_ext is currently broken for Linux v6.2 and so skip for Linux v6.2+
ifeq ($(shell test $(LINUX_VERSION) -lt $(LINUX_VERSION_6_2); echo $$?),0)
obj-m		+= ivc_ext.o
tegra_bpmp-y	+= ../../clk/tegra/clk-bpmp.o
tegra_bpmp-y	+= ../../reset/tegra/reset-bpmp.o
tegra_bpmp-y	+= ../../soc/tegra/powergate-bpmp.o
tegra_bpmp-y	+= bpmp-debugfs.o
tegra_bpmp-y	+= bpmp-tegra186-hv.o
obj-m 		+= tegra_bpmp.o
endif

