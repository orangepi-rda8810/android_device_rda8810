LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

export TARGET_ALL_PARTITIONS_ON_FTL

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/priv_modules
LOCAL_MODULE := rnftl.ko

include $(BUILD_EXTMODULE)

