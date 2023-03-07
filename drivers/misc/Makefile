# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.

LINUX_VERSION := $(shell expr $(VERSION) \* 256 + $(PATCHLEVEL))
LINUX_VERSION_6_3 := $(shell expr 6 \* 256 + 3)

obj-m += mods/
obj-m += nvsciipc/
ifdef CONFIG_PCI
obj-m += tegra-pcie-dma-test.o
endif
# bluedroid_pm is currently broken for Linux v6.3 and so skip for Linux v6.3+
ifeq ($(shell test $(LINUX_VERSION) -lt $(LINUX_VERSION_6_3); echo $$?),0)
obj-m += bluedroid_pm.o
endif
obj-m += nvscic2c-pcie/
