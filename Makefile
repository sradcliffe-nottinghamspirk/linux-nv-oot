# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

obj-m += drivers/
obj-m += sound/soc/tegra/
ifeq ($(shell test $$VERSION -lt 6; echo $$?),0)
obj-m += sound/tegra-safety-audio/
endif
