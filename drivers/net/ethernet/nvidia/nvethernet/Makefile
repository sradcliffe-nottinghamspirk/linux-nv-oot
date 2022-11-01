#SPDX-License-Identifier: GPL-2.0-only
#Copyright (c) 2019-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

OSI_COMMON := nvethernetrm/osi/common
OSI_CORE := nvethernetrm/osi/core
OSI_DMA := nvethernetrm/osi/dma

obj-m += nvethernet.o

ccflags-y += -DLINUX_IVC -DUPDATED_PAD_CAL \
	     -I$(srctree.nvidia)/drivers/net/ethernet/nvidia/nvethernet/nvethernetrm/include \
	     -I$(srctree.nvidia)/drivers/net/ethernet/nvidia/nvethernet/nvethernetrm/osi/common/include

ccflags-y += -DMACSEC_SUPPORT -DNET30 -DDEBUG_MACSEC -DOSI_DEBUG $(if $(CONFIG_TEGRA_EPL),-DHSI_SUPPORT,) -DLINUX_OS -DNVPKCS_MACSEC

nvethernet-objs:= ether_linux.o \
		  osd.o \
		  ethtool.o \
		  ether_tc.o \
		  sysfs.o \
		  ioctl.o \
		  ptp.o \
		  macsec.o \
		  selftests.o \
		  $(OSI_CORE)/osi_core.o \
		  $(OSI_CORE)/osi_hal.o \
		  $(OSI_CORE)/macsec.o \
		  $(OSI_COMMON)/osi_common.o \
		  $(OSI_COMMON)/eqos_common.o \
		  $(OSI_COMMON)/mgbe_common.o \
		  $(OSI_DMA)/osi_dma.o \
		  $(OSI_DMA)/osi_dma_txrx.o \
		  $(OSI_CORE)/eqos_core.o \
		  $(OSI_CORE)/ivc_core.o \
		  $(OSI_CORE)/mgbe_core.o \
		  $(OSI_CORE)/core_common.o \
		  $(OSI_CORE)/xpcs.o \
		  $(OSI_DMA)/mgbe_dma.o \
		  $(OSI_CORE)/eqos_mmc.o \
		  $(OSI_DMA)/eqos_dma.o \
		  $(OSI_DMA)/eqos_desc.o \
		  $(OSI_DMA)/mgbe_desc.o \
		  $(OSI_DMA)/debug.o \
		  $(OSI_CORE)/mgbe_mmc.o \
		  $(OSI_CORE)/frp.o \
		  $(OSI_CORE)/vlan_filter.o \
		  $(OSI_CORE)/debug.o
