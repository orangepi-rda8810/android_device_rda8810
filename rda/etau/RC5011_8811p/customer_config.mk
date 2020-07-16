#value maybe:none,minimal,base,full
TARGET_GOOGLE_APP_LEVEL := base

#here is for adups fota support
#FOTA_UPDATE_SUPPORT := true
#FOTA_UPDATE_WITH_ICON := false
#FOTA_APP_PATH := vendor/3rdparty/adups-fota

GOOLGE_ADDON_PACKAGES := \
    Gmail2 \
    Hangouts \
    Maps \
    Velvet \
    GoogleTTS \
    Keep \
    LatinImeGoogle \
    GenieWidget \
    talkback


CUSTOMER_DISABLE_ROOT := yes

$(call inherit-product, build/target/product/languages_full.mk)
# Get a list of languages.
$(call inherit-product, $(SRC_TARGET_DIR)/product/locales_full.mk)

CUSTOMER_PRODUCT_LOCALES := $(PRODUCT_LOCALES)

CUSTOMER_PACKAGE_OVERLAYS := $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/overlay

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
    ro.apk_install_to_sd=0

#for emulated storage,valid value: nand , sdcard ,notset
PRODUCT_PROPERTY_OVERRIDES += \
    ro.primary.storage=notset

PRODUCT_PROPERTY_OVERRIDES += \
    ro.dim.immediately=true

PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=240

PRODUCT_COPY_FILES += \
    device/rda/common/res/init.storage.sdonly.rc:root/init.storage.rc \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.faketouch.xml:system/etc/permissions/android.hardware.faketouch.xml \

#customer default wallpaper
PRODUCT_COPY_FILES += \
    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/wallpaper/default_wallpaper.jpg:vendor/wallpaper/default_wallpaper.jpg \
    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/wallpaper/default_wallpaper_small.jpg:vendor/wallpaper/default_wallpaper_small.jpg

# customer boot or shutdown anmation
PRODUCT_COPY_FILES += \
    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/initlogo.rle:root/initlogo.rle \

#    $(BOARDDIR)/bootshutdownres/bootanimation.zip:vendor/media/bootanimation.zip \
#    $(BOARDDIR)/bootshutdownres/shutanimation.zip:vendor/media/shutanimation.zip

#customer boot or showdown audio, use default one define in device/rda/common/base.mk.
# PRODUCT_COPY_FILES += \
#    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/poweron.mp3:vendor/media/poweron.mp3 \
#    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/shutdown.mp3:vendor/media/shutdown.mp3

#origin package placed in vendor/3rdparty/misc/preinstallapk
PrebuildInternal_PACKAGES := \
    FlashPlayer \

PrebuildExternal_PACKAGES := \

PRODUCT_COPY_FILES += \
    $(foreach apk,$(PrebuildExternal_PACKAGES),vendor/3rdparty/misc/preinstallapk/$(apk).apk:vendor/preinstall/$(apk).apk) \
    $(foreach apk,$(PrebuildInternal_PACKAGES),vendor/3rdparty/misc/preinstallapk/$(apk).apk:vendor/app/$(apk).apk) \


