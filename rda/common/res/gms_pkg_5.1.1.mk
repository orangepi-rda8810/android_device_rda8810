BOARD_SUPPORT_OPEN_GMS := true
ifeq ($(BOARD_SUPPORT_OPEN_GMS),true)
GOOGLE_ROOT := vendor/3rdparty/google/open_gapps-arm-5.1-stock-20150924/system
ifeq ($(TARGET_GOOGLE_APP_LEVEL),full)
include $(GOOGLE_ROOT)/products/gms.mk
else ifeq ($(TARGET_GOOGLE_APP_LEVEL),base)
include $(GOOGLE_ROOT)/products/gms_base.mk
else ifeq ($(TARGET_GOOGLE_APP_LEVEL),minimal)
include $(GOOGLE_ROOT)/products/gms_mini.mk
endif
else
GOOGLE_ROOT := vendor/3rdparty/google/open_gapps-arm-5.1-stock-20150924/system
#google app base service,don't remove
GoogleServices_base_PACKAGES := \
    ./priv-app/GoogleServicesFramework \
    ./priv-app/GoogleLoginService \

#sync support,may remove for reduce memory
GoogleServices_extra_PACKAGES := \
    ./priv-app/GoogleBackupTransport.apk  \
    ./priv-app/GoogleOneTimeInitializer \
    ./priv-app/GoogleFeedback \
    ./priv-app/GooglePartnerSetup \
    ./app/GoogleContactsSyncAdapter  \
    ./app/GoogleCalendarSyncAdapter

GoogleApp_PACKAGES :=

ifneq ($(TARGET_GOOGLE_APP_LEVEL),minimal)
GoogleServices_extra_PACKAGES += \
    ./priv-app/Phonesky \
    ./priv-app/PrebuiltGmsCore \

GoogleServices_extra_PACKAGES += \
    $(GOOLGE_ADDON_PACKAGES)

GoogleApp_PACKAGES += \

endif
ifeq ($(TARGET_GOOGLE_APP_LEVEL),full)
GoogleServices_extra_PACKAGES += \
    ./app/Books \
    ./app/CalendarGooglePrebuilt \
    ./app/Chrome \
    ./app/CloudPrint2 \
    ./app/DeskClockGoogle \
    ./app/Drive \
    ./app/EditorsDocs \
    ./app/EditorsSheets \
    ./app/EditorsSlides \
    ./app/FaceLock \
    ./app/FitnessPrebuilt \
    ./app/GoogleCamera \
    ./app/GoogleEars \
    ./app/GoogleEarth \
    ./app/GoogleHome \
    ./app/GoogleTTS \
    ./app/LatinImeGoogle \
    ./app/Maps \
    ./app/Music2 \
    ./app/Newsstand \
    ./app/Photos \
    ./app/PlayGames \
    ./app/PlusOne \
    ./app/PrebuiltBugle \
    ./app/PrebuiltExchange3Google \
    ./app/PrebuiltNewsWeather \
    ./app/Street \
    ./app/Videos \
    ./app/WebViewGoogle \
    ./app/YouTube

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

ifneq ($(TARGET_GOOGLE_APP_LEVEL),none)
PRODUCT_COPY_FILES += \
    $(foreach apk,$(GoogleApp_Internal_PACKAGES), $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/$(apk),vendor/$(apk)) ) \
    $(foreach apk,$(GoogleApp_External_PACKAGES), $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/$(apk),vendor/preinstall/$(apk)) ) \
    $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/etc/permissions,system/etc/permissions) \
    $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/framework,system/framework)
endif

ifeq ($(TARGET_GOOGLE_APP_LEVEL),full)
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,$(GOOGLE_ROOT)/lib,vendor/lib)
endif
endif
