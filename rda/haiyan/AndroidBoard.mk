LOCAL_PATH := $(call my-dir)

# Compile U-Boot
ifneq ($(strip $(TARGET_NO_BOOTLOADER)),true)

INSTALLED_UBOOT_TARGET := $(PRODUCT_OUT)/bootloader.img
include u-boot/AndroidUBoot.mk

endif # End of U-Boot

# Compile Linux Kernel
ifneq ($(strip $(TARGET_NO_KERNEL)),true)
INSTALLED_KERNEL_TARGET := $(PRODUCT_OUT)/kernel
include kernel/AndroidKernel.mk

file := $(INSTALLED_KERNEL_TARGET)
ALL_PREBUILT += $(file)
$(file) : $(TARGET_PREBUILT_KERNEL) | $(ACP)
	$(transform-prebuilt-to-target)

endif # End of Kernel

#compile modem
ifneq ($(strip $(TARGET_NO_MODEM)),true)
INSTALLED_MODEM_TARGET := $(PRODUCT_OUT)/modem.img

MODEM_MK := modem/modem-2g/AndroidModem.mk

ifeq ($(strip $(MODEM_TYPE)),dualmode-2g)
MODEM_MK := modem/modem-dualmode-2g/AndroidModem.mk
endif

ifeq ($(strip $(MODEM_TYPE)),dualmode-3g)
MODEM_MK := modem/modem-dualmode-3g/AndroidModem.mk
endif


# compatibility with old modem directory
CHECK_EXISTS = $(shell if [ -f $(MODEM_MK) ];then echo "true";fi)
ifeq ($(strip $(CHECK_EXISTS)),)
MODEM_MK := modem/AndroidModem.mk
endif
include $(MODEM_MK)
endif
