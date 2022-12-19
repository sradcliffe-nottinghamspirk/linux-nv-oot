# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

ccflags-y += -Werror
ccflags-y += -DDYNAMIC_DEBUG_MODULE
ccflags-y += -I$(srctree.nvidia-oot)/drivers/misc/nvscic2c-pcie
#host1x-next and Tegra DRM are not moved to nvidia-oot include path yet.
ccflags-y += -I$(srctree.nvidia)/drivers/gpu/host1x/include
ccflags-y += -I$(srctree.nvidia)/drivers/gpu/drm/tegra/include

ifdef CONFIG_PCIE_TEGRA194_EP
obj-m := nvscic2c-pcie-epc.o nvscic2c-pcie-epf.o
nvscic2c-pcie-epc-y := comm-channel.o dt.o endpoint.o epc/module.o iova-alloc.o iova-mngr.o pci-client.o stream-extensions.o vmap.o vmap-pin.o
nvscic2c-pcie-epf-y := comm-channel.o dt.o endpoint.o epf/module.o iova-alloc.o iova-mngr.o pci-client.o stream-extensions.o vmap.o vmap-pin.o
endif
