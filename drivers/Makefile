# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2022-2023, NVIDIA CORPORATION.  All rights reserved.

LINUXINCLUDE += -I$(srctree.nvidia-oot)/include

obj-m += block/tegra_virt_storage/
ifdef CONFIG_PSTORE
obj-m += block/tegra_oops_virt_storage/
endif
obj-m += c2c/
obj-m += clink/
obj-m += cpuidle/
obj-m += crypto/
obj-m += debug/
ifdef CONFIG_PM_DEVFREQ
obj-m += devfreq/
endif
obj-m += firmware/tegra/
ifdef CONFIG_TEGRA_HOST1X
obj-m += gpu/
endif
obj-m += hwmon/
ifdef CONFIG_I2C
obj-m += i2c/busses/
obj-m += mfd/
endif
ifdef CONFIG_MTD
obj-m += mtd/devices/
endif
obj-m += misc/
obj-m += net/
obj-m += nvpps/
obj-m += nvpmodel/
ifdef CONFIG_PCI
obj-m += pci/
endif
obj-m += pinctrl/
obj-m += platform/tegra/
obj-m += pwm/
obj-m += ras/
obj-m += rtc/
obj-m += cpuidle/
ifdef CONFIG_SCSI
obj-m += scsi/ufs/
endif
obj-m += soc/tegra/
ifdef CONFIG_SPI
obj-m += spi/
endif
ifdef CONFIG_I2C
obj-m += thermal/
endif
obj-m += watchdog/
ifdef CONFIG_TEGRA_HOST1X
obj-m += video/tegra/
endif
obj-m += virt/tegra/
ifdef CONFIG_TEGRA_HOST1X
obj-m += media/
endif
obj-m += staging/platform/tegra/gte/
obj-m += staging/platform/tegra/gte_test/

