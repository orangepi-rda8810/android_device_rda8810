LOCAL_PATH:= $(call my-dir)

#
# Build Crash listner
#
include $(CLEAR_VARS)

LOCAL_MODULE := rda_crash_listner
LOCAL_SRC_FILES:= \
    rda_crash_listener.c
LOCAL_C_INCLUDES += \
    bionic \
    $(LOCAL_PATH)
LOCAL_MODULE_TAGS := eng debug
LOCAL_SHARED_LIBRARIES := \
    liblog \

include $(BUILD_EXECUTABLE)


#
# Build test tool
#
include $(CLEAR_VARS)

LOCAL_MODULE := test-modem_crash_event
LOCAL_SRC_FILES:= \
    test-modem_crash_event.c
LOCAL_C_INCLUDES += \
    bionic \
    $(LOCAL_PATH)
LOCAL_MODULE_TAGS := tests
LOCAL_SHARED_LIBRARIES := \
    liblog \

include $(BUILD_EXECUTABLE)


#
# Install modemcored daemon.
#
include $(CLEAR_VARS)

LOCAL_MODULE := modemcored.sh
LOCAL_SRC_FILES:= modemcored.sh
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := eng debug

include $(BUILD_PREBUILT)

