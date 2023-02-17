ccflags-y += -Wno-implicit-fallthrough
ccflags-y += -I$(srctree.nvidia)/drivers/gpu/host1x/hw/
ccflags-y += -I$(srctree.nvidia)/drivers/gpu/host1x/include
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -I$(srctree)/include

ifdef CONFIG_TEGRA_HOST1X
obj-m += tegra-se-nvhost.o
obj-m += tegra-hv-vse-safety.o
obj-m += tegra-nvvse-cryptodev.o
endif
obj-m += tegra-se-nvrng.o
