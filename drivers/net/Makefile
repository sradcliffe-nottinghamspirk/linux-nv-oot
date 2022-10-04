# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

obj-m += ethernet/
obj-m += tegra_hv_net.o
ifeq ($(shell test $$VERSION -lt 6; echo $$?),0)
obj-m += can/
endif
