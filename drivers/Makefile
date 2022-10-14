# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia-oot)/include

obj-m += block/tegra_virt_storage/
obj-m += c2c/
obj-m += clink/
obj-m += crypto/
obj-m += devfreq/
obj-m += firmware/tegra/
obj-m += gpu/
obj-m += hwmon/
obj-m += i2c/busses/
obj-m += mfd/
obj-m += misc/
obj-m += net/
obj-m += nvpps/
obj-m += nvpmodel/
obj-m += pci/
obj-m += pinctrl/
obj-m += platform/tegra/
obj-m += pwm/
obj-m += ras/
obj-m += rtc/
obj-m += scsi/ufs/
obj-m += soc/tegra/
obj-m += spi/
obj-m += thermal/
obj-m += watchdog/
obj-m += video/tegra/
obj-m += virt/tegra/
ifeq ($(shell test $$VERSION -lt 6; echo $$?),0)
obj-m += media/
endif

