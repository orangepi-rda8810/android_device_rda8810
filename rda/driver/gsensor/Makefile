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

obj-m := rda_gs.o
ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MMA7660)
rda_gs-objs = mma7660.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MMA865X)
rda_gs-objs = mma865x.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MMA845X)
rda_gs-objs = mma845x.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_STK8312)
rda_gs-objs = stk8312.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MC3XXX)
rda_gs-objs = mc3xxx.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MM3A310)
rda_gs-objs = mm3a310.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MXC622X)
rda_gs-objs = mxc622x.o
ifneq ($(RDA_GS_MXC622X_MODE),)
EXTRA_CFLAGS += -D$(RDA_GS_MXC622X_MODE)
endif
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MXC622X_LS)
rda_gs-objs = mxc622x_ls.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MXC6225)
rda_gs-objs = mxc6225.o
else ifeq ($(RDA_CUSTOMER_DRV_GS), GS_MXC6255)
rda_gs-objs = mxc6255.o
ifneq ($(RDA_GS_MXC6255_MODE),)
EXTRA_CFLAGS += -D$(RDA_GS_MXC6255_MODE)
endif
else
# a skeleton driver does nothing, just for no gsensor build.
rda_gs-objs = skeleton.o
endif

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
