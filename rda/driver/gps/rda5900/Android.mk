LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/driver
LOCAL_MODULE := rda_gps.ko

include $(BUILD_EXTMODULE)

