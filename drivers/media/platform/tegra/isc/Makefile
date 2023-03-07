GCOV_PROFILE := y

LINUX_VERSION := $(shell expr $(VERSION) \* 256 + $(PATCHLEVEL))
LINUX_VERSION_6_3 := $(shell expr 6 \* 256 + 3)

# isc_gpio/mgr is currently broken for Linux v6.3 and so skip for Linux v6.3+
ifeq ($(shell test $(LINUX_VERSION) -lt $(LINUX_VERSION_6_3); echo $$?),0)
obj-m	+= isc_gpio.o
obj-m	+= isc_mgr.o
endif
obj-m	+= isc_dev.o
obj-m	+= isc_pwm.o
