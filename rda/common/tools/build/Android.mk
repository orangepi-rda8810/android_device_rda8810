LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := build_header.c
LOCAL_MODULE := build_header
LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_HOST_EXECUTABLE)

