LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifneq ($(RDA_CUSTOMER_DRV_LS), LS_NONE)
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/driver
LOCAL_MODULE := rda_ls.ko
include $(BUILD_EXTMODULE)
endif


