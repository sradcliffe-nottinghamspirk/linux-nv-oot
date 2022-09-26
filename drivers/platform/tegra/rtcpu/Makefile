# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia-oot)/include
LINUXINCLUDE += -I$(srctree)/include
LINUXINCLUDE += -I$(srctree.nvidia-oot)/drivers/platform/tegra/rtcpu
LINUXINCLUDE += -I$(srctree.nvidia-oot)/drivers/platform/tegra

obj-m += capture-ivc.o
obj-m += ivc-bus.o
obj-m += camchar.o
obj-m += camera-diagnostics.o
obj-m += debug.o
obj-m += clk-group.o
obj-m += hsp-mailbox-client.o
obj-m += tegra-rtcpu-trace.o
obj-m += rtcpu-monitor.o
obj-m += reset-group.o
obj-m += device-group.o
