###########################################################
## Standard rules for copying files that are prebuilt
##
## Additional inputs from base_rules.make:
## None.
##
###########################################################

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := $(TARGET_OUT_INTERMEDIATES)/$(LOCAL_PATH)/$(LOCAL_MODULE)

RDA_TARGET_DIR := $(ANDROID_BUILD_TOP)/$(RDA_TARGET_DEVICE_DIR)

my_prefix := TARGET_

OVERRIDE_BUILT_MODULE_PATH := $($(my_prefix)OUT_INTERMEDIATE_LIBRARIES)

include $(BUILD_SYSTEM)/base_rules.mk

.PHONY: $(LOCAL_SRC_FILES)
$(LOCAL_BUILT_MODULE) : $(LOCAL_SRC_FILES) | $(ACP)
	$(transform-prebuilt-to-target)
	$(transform-ranlib-copy-hack)

.PHONY: $(TARGET_PREBUILT_KERNEL)
$(LOCAL_SRC_FILES) : $(TARGET_PREBUILT_KERNEL)
	$(hide) echo 'build $@'
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -a $(patsubst $(TARGET_OUT_INTERMEDIATES)/%,%,$(dir $@))/* $(dir $@)/
	$(hide) rm -f $(dir $@)/Android.mk
	$(hide) $(MAKE) -C $(dir $@) TARGET_DIR=$(RDA_TARGET_DIR)

