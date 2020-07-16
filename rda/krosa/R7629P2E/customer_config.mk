#value maybe:none,minimal,base,full
TARGET_GOOGLE_APP_LEVEL := full

#here is for adups fota support
#FOTA_UPDATE_SUPPORT := true
#FOTA_UPDATE_WITH_ICON := false
#FOTA_APP_PATH := vendor/3rdparty/adups-fota


PRODUCT_PROPERTY_OVERRIDES += \
    ro.board.has.gps=false \

ifeq "$(strip ${PLATFORM_VERSION})" "4.4.2"
PRODUCT_PROPERTY_OVERRIDES += capability_sim_slot_count=2
else
PRODUCT_PROPERTY_OVERRIDES += persist.radio.multisim.config=dsds
endif

#to generate odex file to sdcard,for reduce userdata usage
PRODUCT_PROPERTY_OVERRIDES += \
    ro.odex.on.sdcard=0
#The third APKs install path setting
PRODUCT_PROPERTY_OVERRIDES += \
    ro.apk_install_to_sd=1

PRODUCT_COPY_FILES += \
    device/rda/common/res/init.storage.normal.rc:root/init.storage.rc

#customer default wallpaper
#PRODUCT_COPY_FILES += \
#    $(BOARDDIR)/wallpaper/default_wallpaper.jpg:vendor/wallpaper/default_wallpaper.jpg \
#    $(BOARDDIR)/wallpaper/default_wallpaper_small.jpg:vendor/wallpaper/default_wallpaper_small.jpg

# customer boot or shutdown anmation
#PRODUCT_COPY_FILES += \
#    $(BOARDDIR)/bootshutdownres/bootanimation.zip:vendor/media/bootanimation.zip \
#    $(BOARDDIR)/bootshutdownres/shutanimation.zip:vendor/media/shutanimation.zip

#customer boot or showdown audio
PRODUCT_COPY_FILES += \
    $(BOARDDIR)/poweron.mp3:vendor/media/poweron.mp3 \
    $(BOARDDIR)/shutdown.mp3:vendor/media/shutdown.mp3


#origin package placed in vendor/3rdparty/misc/preinstallapk
PrebuildInternal_PACKAGES := \
    GoogleMaps \
    FlashPlayer

PrebuildExternal_PACKAGES := \

PRODUCT_COPY_FILES += \
    $(foreach apk,$(PrebuildExternal_PACKAGES),vendor/3rdparty/misc/preinstallapk/$(apk).apk:vendor/preinstall/$(apk).apk) \
    $(foreach apk,$(PrebuildInternal_PACKAGES),vendor/3rdparty/misc/preinstallapk/$(apk).apk:vendor/app/$(apk).apk) \

