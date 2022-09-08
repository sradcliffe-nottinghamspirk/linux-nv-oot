# SPDX-FileCopyrightText: Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

objtree = $(srctree)

# Redefine the fixdep command
cmd_and_fixdep =                                                             \
	$(cmd);                                                              \
	$(objtree)/scripts/basic/fixdep $(depfile) $@ '$(make-cmd)' > $(dot-target).cmd;\
	rm -f $(depfile)

include $(oottree)/scripts/Makefile.lib

DTC_CPP_FLAGS += $(EXTRA_CPP_FLAGS)

DTC_CPP_FLAGS += -DLINUX_VERSION=504 -DTEGRA_HOST1X_DT_VERSION=1

tegra-dtstree := $(TOP)/hardware/nvidia

# From out/l4t-t186ref-debug-aarch64/nvidia/kernel-nvidia-oot/nvidia-oot/device-tree/platform
tegra-rel-dtstree := ../../../../../../../hardware/nvidia

DTC_INCLUDE :=
# SOC independent common include
DTC_INCLUDE += $(tegra-dtstree)/soc/tegra/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/platform/tegra/common/kernel-dts

# SOC T23X specific common include
DTC_INCLUDE += $(tegra-dtstree)/soc/t23x/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/soc/t23x/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t23x/common/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t23x/automotive/kernel-dts/common/linux/

dtb-oot-y :=
dtbo-oot-y :=

DTB_LIST := $(dtb-oot-y)
DTBO_LIST := $(dtbo-oot-y)
dtb-oot-y :=
dts_makefile=$(foreach d,$(wildcard $1*), $(call dts_makefile,$(d)/,$(2)) $(if $(findstring Makefile,$(d)),$(d)))
dts_mfiles = $(call dts_makefile, $(tegra-dtstree)/platform/t23x, Makefile)

ifneq ($(dts_mfiles),)
dts-include :=
include $(dts_mfiles)
dtb-oot-y := $(addprefix $(tegra-rel-dtstree)/,$(dtb-oot-y))
dtbo-oot-y := $(addprefix $(tegra-rel-dtstree)/,$(dtbo-oot-y))
ifneq ($(dts-include),)
DTC_INCLUDE += $(addprefix $(tegra-dtstree)/,$(dts-include))
endif
endif

DTB_LIST += $(dtb-oot-y)
DTBO_LIST += $(dtbo-oot-y)
DTB_NEW_RULE_LIST := $(dtb-oot-y)
DTBO_NEW_RULE_LIST := $(dtbo-oot-y)

# Now save DTB_LIST to dtb-oot-y
dtb-oot-y := $(DTB_LIST)
dtbo-oot-y := $(DTBO_LIST)

# Add path of main Makefile to each dtb/dtbo list
DTB_OBJS := $(addprefix $(obj)/,$(DTB_LIST))
DTBO_OBJS := $(addprefix $(obj)/,$(DTBO_LIST))

###
# remove the ../
replace_ddot = $(subst ../,,$(1))

define _define_dtb_rule
$(obj)/$(call replace_ddot,$(1)): $(src)/$(patsubst %.dtb,%.dts,$(1)) FORCE
endef

$(foreach _dtb, $(DTB_NEW_RULE_LIST), $(eval $(call _define_dtb_rule,$(_dtb))))

DTB_OBJS := $(call replace_ddot,$(DTB_OBJS))
DTB_NEW_RULE_LIST := $(addprefix $(obj)/,$(DTB_NEW_RULE_LIST))
DTB_NEW_RULE_LIST := $(call replace_ddot,$(DTB_NEW_RULE_LIST))

$(DTB_NEW_RULE_LIST):
	$(call if_changed_dep,dtc)

define _define_dtbo_rule
$(obj)/$(call replace_ddot,$(1)): $(src)/$(patsubst %.dtbo,%.dts,$(1)) FORCE
endef

$(foreach _dtbo, $(DTBO_NEW_RULE_LIST), $(eval $(call _define_dtbo_rule,$(_dtbo))))

DTBO_OBJS := $(call replace_ddot,$(DTBO_OBJS))
DTBO_NEW_RULE_LIST := $(addprefix $(obj)/,$(DTBO_NEW_RULE_LIST))
DTBO_NEW_RULE_LIST := $(call replace_ddot,$(DTBO_NEW_RULE_LIST))

$(DTBO_NEW_RULE_LIST):
	$(call if_changed_dep,dtc)

DTBS_DTBOS := $(DTB_OBJS)$(DTBO_OBJS)

dtbs: $(DTB_OBJS) $(DTBO_OBJS) FORCE
	if [ ! -d $(obj)/dtbs/ ] ; then \
		mkdir -p $(obj)/dtbs/ ; \
	fi
	if [ ! -z $(DTBS_DTBOS) ] ; then \
		cp -u $(DTB_OBJS) $(DTBO_OBJS) $(obj)/dtbs/ ; \
	fi
	if [ -d $(obj)/hardware/ ] ; then \
		rm -rf $(obj)/hardware/ ; \
	fi

clean-files := *.dtb *.dtbo *.tmp
