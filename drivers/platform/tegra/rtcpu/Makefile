# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

LINUX_VERSION := $(shell expr $(VERSION) \* 256 + $(PATCHLEVEL))
LINUX_VERSION_6_2 := $(shell expr 6 \* 256 + 2)

# capture-ivc is currently broken for Linux v6.2 and so skip for Linux v6.2+
ifeq ($(shell test $(LINUX_VERSION) -lt $(LINUX_VERSION_6_2); echo $$?),0)
obj-m += capture-ivc.o
endif
obj-m += ivc-bus.o
obj-m += camchar.o
obj-m += camera-diagnostics.o
obj-m += rtcpu-debug.o
obj-m += clk-group.o
obj-m += hsp-mailbox-client.o
obj-m += reset-group.o
obj-m += device-group.o

tegra-camera-rtcpu-objs :=  tegra-camera-rtcpu-base.o tegra-rtcpu-trace.o rtcpu-monitor.o
obj-m += tegra-camera-rtcpu.o
