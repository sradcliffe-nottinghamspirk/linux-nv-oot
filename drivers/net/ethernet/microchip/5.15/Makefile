# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the Microchip network device drivers.
#

obj-m += lan743x.o

lan743x-objs := lan743x_main.o lan743x_ethtool.o lan743x_ptp.o

all:
	make W=1 -C /lib/modules/`uname -r`/build M=`pwd` modules
