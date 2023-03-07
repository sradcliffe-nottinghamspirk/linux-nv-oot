# SPDX-FileCopyrightText: Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

#
# NVPPS driver and PTP Framework.
#

LINUX_VERSION := $(shell expr $(VERSION) \* 256 + $(PATCHLEVEL))
LINUX_VERSION_6_3 := $(shell expr 6 \* 256 + 3)

# nvpps is currently broken for Linux v6.3 and so skip for Linux v6.3+
ifeq ($(shell test $(LINUX_VERSION) -lt $(LINUX_VERSION_6_3); echo $$?),0)
obj-m += nvpps.o
endif
nvpps-y += ../platform/tegra/ptp-notifier.o
nvpps-y += nvpps_main.o
