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
    ro.apk_install_to_sd=1

#for emulated storage,valid value: nand , sdcard ,notset
PRODUCT_PROPERTY_OVERRIDES += \
    ro.primary.storage=sdcard

PRODUCT_PROPERTY_OVERRIDES += \
    ro.dim.immediately=true

PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=240

PRODUCT_COPY_FILES += \
    device/rda/common/res/init.storage.normal.rc:root/init.storage.rc

#customer default wallpaper
PRODUCT_COPY_FILES += \
    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/wallpaper/default_wallpaper.jpg:vendor/wallpaper/default_wallpaper.jpg \
    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/wallpaper/default_wallpaper_small.jpg:vendor/wallpaper/default_wallpaper_small.jpg

# customer boot or shutdown anmation
PRODUCT_COPY_FILES += \
    device/rda/common/res/bootlogo/initlogo_wvga.rle:root/initlogo.rle \

#    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/bootshutdownres/bootanimation.zip:vendor/media/bootanimation.zip \
#    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/bootshutdownres/shutanimation.zip:vendor/media/shutanimation.zip


#customer boot or showdown audio,use default one define in device/rda/common/base.mk.
# PRODUCT_COPY_FILES += \
#    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/poweron.mp3:vendor/media/poweron.mp3 \
#    $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/shutdown.mp3:vendor/media/shutdown.mp3

#for cdrom
PRODUCT_PROPERTY_OVERRIDES += \
	ro.sys.usb.cdrom=yes

PRODUCT_COPY_FILES += \
	device/rda/common/res/cdrom.iso:vendor/cdrom/rdarun.iso

#note:def_autorun_list is a black list; def_backgroundrun_list is a white list
PRODUCT_PROPERTY_OVERRIDES += \
    ro.def_autorun_list=com.bbm:com.facebook.katana:com.android.vending:com.twitter.android \
    ro.def_autorun_list_1=com.google.android.apps.maps:com.google.android.gsf.login \
    ro.def_autorun_list_2=com.google.android.music:com.google.android.feedback \
    ro.def_autorun_list_3=com.google.android.backuptransport:com.google.android.youtube:com.google.android.apps.plus \
    ro.def_autorun_list_4=com.google.android.videos:com.google.android.partnersetup:com.google.android.talk\
    ro.def_autorun_list_5=com.google.android.apps.magazines:com.google.android.configupdater \
    ro.def_autorun_list_6=com.google.android.apps.genie.geniewidget:com.google.android.street:com.google.android.gm \
    ro.def_autorun_list_7=com.google.android.googlequicksearchbox:com.google.android.tts \
    ro.def_autorun_list_8=com.google.android.ears:com.google.android.apps.books:com.google.android.apps.uploader \
    ro.def_autorun_list_9=com.google.android.play.games:com.google.android.marvin.talkback \
    ro.def_autorun_list_10=com.google.android.gms:com.google.android.syncadapters.contacts \
    ro.def_backgroundrun_list=none \

#origin package placed in vendor/3rdparty/misc/preinstallapk
PrebuildInternal_PACKAGES := \
	FlashPlayer \

PrebuildExternal_PACKAGES := \

PRODUCT_COPY_FILES += \
    $(foreach apk,$(PrebuildExternal_PACKAGES),vendor/3rdparty/misc/preinstallapk/$(apk).apk:vendor/preinstall/$(apk).apk) \
    $(foreach apk,$(PrebuildInternal_PACKAGES),vendor/3rdparty/misc/preinstallapk/$(apk).apk:vendor/app/$(apk).apk) \

