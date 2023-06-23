# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia-oot)/include

obj-m += drivers/

ifdef CONFIG_SOUND
obj-m += sound/soc/tegra/
obj-m += sound/tegra-safety-audio/
obj-m += sound/soc/tegra-virt-alt/
endif
