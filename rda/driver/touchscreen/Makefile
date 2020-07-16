
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

PANEL_COMM_DIR = $(TARGET_DIR)/../../driver/panel

EXTRA_CFLAGS += -I$(TARGET_DIR)/include -I$(PANEL_COMM_DIR) -fno-pic
EXTRA_CFLAGS += -Werror

# linux build system integration

ifneq ($(KERNELRELEASE),)
# Inside the kernel build system

obj-m := rda_ts.o
rda_ts-objs := rda_ts_base.o

#obj-y := gsl_point_id.o
#obj-y := rda_ts_gsl168x.o
#$(obj)/gsl_point_id.o: $(srctree)/$(obj)/gsl_point_id
#	cp $(srctree)/$(obj)/gsl_point_id  $(obj)/gsl_point_id.o

#obj-y += gsl_point_id.o
ifneq ($(findstring TS_,${RDA_CUSTOMER_DRV_TS}),)
ifneq ($(findstring TS_GSL168X,${RDA_CUSTOMER_DRV_TS}),)
ifeq ($(GSL168X_MODEL), GSL168X_MODEL_70_WVGAL_BP605)
#rda_ts-objs += gsl_point_id.o
endif
#obj-y := rda_ts_gsl168x.o
rda_ts-objs += rda_ts_gsl168x.o
rda_ts-objs += gsl_point_id
EXTRA_CFLAGS += -D$(GSL168X_MODEL)
EXTRA_CFLAGS += -DGSL168X_MODEL
endif
ifneq ($(findstring TS_MSG2133,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_msg2133.o
ifeq ($(TS_MSG2133_FW_UPDATE), YES)
rda_ts-objs += msg2133_auto_update_firmware.o
EXTRA_CFLAGS += -DTS_FIRMWARE_UPDATE
endif
ifneq ($(TS_MSG2133_MODEL), )
EXTRA_CFLAGS += -D$(TS_MSG2133_MODEL)
endif
EXTRA_CFLAGS += -DMSG2133_MODEL
endif
ifneq ($(findstring TS_FT6X06,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_ft6x06.o
EXTRA_CFLAGS += -DFT6X06_MODEL
ifneq ($(TS_FT6X06_MODEL),)
EXTRA_CFLAGS += -D$(TS_FT6X06_MODEL)
endif
endif
ifneq ($(findstring TS_NT11004,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_nt11004.o
EXTRA_CFLAGS += -DNT11004_MODEL
endif
ifneq ($(findstring TS_IT7252,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_it7252.o
EXTRA_CFLAGS += -DIT7252_MODEL
endif
ifneq ($(findstring TS_GTP868,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_gtp868.o
EXTRA_CFLAGS += -DGTP868_MODEL
endif
ifneq ($(findstring TS_GTP960,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_gtp960.o
EXTRA_CFLAGS += -DGTP960_MODEL
endif
ifneq ($(findstring TS_ICN831X,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_icn831x.o
EXTRA_CFLAGS += -DICN831X_MODEL
endif
ifneq ($(TS_MODEL_CUSTOMER),)
EXTRA_CFLAGS += -D$(TS_MODEL_CUSTOMER)
endif
ifneq ($(findstring TS_EKT2527,${RDA_CUSTOMER_DRV_TS}),)
rda_ts-objs += rda_ts_elan.o
EXTRA_CFLAGS += -DEKT2527_MODEL
endif
else
$(error output "$(RDA_CUSTOMER_DRV_TS)" does not match any ts))
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
