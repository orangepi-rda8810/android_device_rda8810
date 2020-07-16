BOARD_SUPPORT_GOOGLE_GMS := true

ifeq ($(BOARD_SUPPORT_GOOGLE_GMS),true)
GOOGLE_ROOT := vendor/3rdparty/google/gms-oem-kkmr1-4.4.2-signed-r7-20150508-1289630/google
ifeq ($(TARGET_GOOGLE_APP_LEVEL),full)
include $(GOOGLE_ROOT)/products/gms.mk
else ifeq ($(TARGET_GOOGLE_APP_LEVEL),base)
include $(GOOGLE_ROOT)/products/gms_base.mk
else ifeq ($(TARGET_GOOGLE_APP_LEVEL),minimal)
include $(GOOGLE_ROOT)/products/gms_mini.mk
endif
else
GOOGLE_ROOT := vendor/3rdparty/google/gapps-kk-20140606-signed/system/

#google app base service,don't remove
GoogleServices_base_PACKAGES := \
    ./priv-app/GoogleServicesFramework.apk  \
    ./priv-app/GoogleLoginService.apk
#sync support,may remove for reduce memory
GoogleServices_extra_PACKAGES := \
    ./priv-app/GoogleBackupTransport.apk  \
    ./app/GoogleContactsSyncAdapter.apk  \
    ./app/GoogleCalendarSyncAdapter.apk

GoogleApp_PACKAGES :=

ifneq ($(TARGET_GOOGLE_APP_LEVEL),minimal)
GoogleServices_extra_PACKAGES += \
    ./priv-app/talkback.apk \
    ./priv-app/GoogleFeedback.apk \
    ./priv-app/GooglePartnerSetup.apk \
    ./priv-app/ConfigUpdater.apk \
    ./app/GmsCore.apk \
    ./app/Phonesky.apk \
    ./app/Gmail2.apk \

GoogleApp_LIBS += \
    libAppDataSearch.so \
    libgames_rtmp_jni.so \
    libgcastv2_base.so \
    libgcastv2_support.so \
    libjgcastservice.so

GoogleApp_PACKAGES += \

endif
ifeq ($(TARGET_GOOGLE_APP_LEVEL),full)
GoogleServices_extra_PACKAGES += \
    ./app/GenieWidget.apk \
    ./app/GoogleEars.apk \
    ./app/Hangouts.apk \
    ./app/MediaUploader.apk \
    ./app/Books.apk \
    ./app/Magazines.apk \
    ./app/PlayGames.apk \
    ./app/YouTube.apk \
    ./app/Maps.apk \
    ./app/PlusOne.apk \
    ./app/GoogleTTS.apk \
    ./app/Street.apk \
    ./app/Music2.apk \
    ./app/Videos.apk \
    ./app/Velvet.apk \

GoogleApp_LIBS += \
    libvcdecoder_jni.so \
    libgoogle_recognizer_jni_l.so \
    libgoogle_hotword_jni.so


#    ./priv-app/SetupWizard.apk \

GoogleApp_PACKAGES += \

endif

GoogleApp_Internal_PACKAGES := \
    $(GoogleServices_base_PACKAGES) \
    $(GoogleServices_extra_PACKAGES)

ifeq ($(TARGET_GOOGLE_APP_LEVEL),full)
GoogleApp_Internal_PACKAGES += \
    $(GoogleApp_PACKAGES)
else ifeq ($(TARGET_GOOGLE_APP_LEVEL),minimal)
GoogleApp_External_PACKAGES := \
     $(GoogleApp_PACKAGES)
else
GoogleApp_External_PACKAGES :=
endif

PRODUCT_COPY_FILES += \
    $(foreach apk,$(GoogleApp_Internal_PACKAGES),$(GOOGLE_ROOT)/$(apk):vendor/$(apk)) \
    $(foreach libs,$(GoogleApp_LIBS),$(GOOGLE_ROOT)/lib/$(libs):vendor/lib/$(libs)) \
    $(foreach apk,$(GoogleApp_External_PACKAGES),$(GOOGLE_ROOT)/$(apk):vendor/preinstall/$(apk)) \
    $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/etc/permissions,system/etc/permissions) \
    $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/framework,system/framework) \

ifeq ($(TARGET_GOOGLE_APP_LEVEL),full)
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/lib,vendor/lib)
endif
endif
