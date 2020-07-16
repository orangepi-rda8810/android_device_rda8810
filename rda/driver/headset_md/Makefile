#
# Copyright (C) 2010-2012 ARM Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
#
# A copy of the licence is included with the program, and can also be obtained from Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#


# For each arch check: CROSS_COMPILE , KDIR , CFLAGS += -DARCH

ARCH ?= arm
#BUILD ?= debug
BUILD ?=
KDIR ?= $(ANDROID_PRODUCT_OUT)/obj/KERNEL
OUTDIR ?= $(ANDROID_PRODUCT_OUT)/obj/KERNEL
PWD :=$(shell pwd)
#MOD :=$(patsubst $(ANDROID_BUILD_TOP)/%,../%,$(PWD))

ifeq ($(BUILD), debug)
EXTRA_CFLAGS += -DDEBUG
endif

EXTRA_CFLAGS += -I$(TARGET_DIR)/include -fno-pic
EXTRA_CFLAGS += -Werror

# linux build system integration

ifneq ($(KERNELRELEASE),)
# Inside the kernel build system

obj-m := rda_headset.o

else
# Outside the kernel build system

# when compiling for ARM we're cross compiling
COMPILER = arm-eabi-

# Validate selected config
all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KDIR) M=$(PWD) INSTALL_MOD_STRIP=1 modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KDIR) M=$(PWD) clean
endif
