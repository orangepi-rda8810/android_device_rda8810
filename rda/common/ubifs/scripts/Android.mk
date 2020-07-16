LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mkubifsimg.sh
LOCAL_SRC_FILES := mkubifsimg.sh
LOCAL_MODULE_CLASS := EXECUTABLES
# We don't need any additional suffix.
LOCAL_MODULE_SUFFIX :=
LOCAL_BUILT_MODULE_STEM := $(notdir $(LOCAL_SRC_FILES))
LOCAL_IS_HOST_MODULE := true

include $(BUILD_PREBUILT)
