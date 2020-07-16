#
# Copyright (C) 2007 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This is a generic phone product that isn't specialized for a specific device.
# It includes the base Android platform.

PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=131072

PRODUCT_PACKAGES := \
    Launcher3 \
    Dialer \
    Mms \
    WAPPushManager \
    Stk1 \
    Stk \
    StkSelection \
    hostapd \
    ApplicationsProvider \
    wpa_supplicant.conf

ifneq "$(strip ${PLATFORM_VERSION})" "4.4.2"
PRODUCT_PACKAGES += \
    CarrierConfig \
    CallLogBackup \
    messaging
endif

PRODUCT_PACKAGES += \
    aapt \
    wpa_supplicant

PRODUCT_PACKAGES += \
    rild \
    ril_ctl \
    ril_client \
    gsmMuxd \
    savelogd \
    usb_traced \
    AT_transfer \
    chat \
    EngineerMode \
    Factorytest \
    libengmodjni \
    libemwifi_jni \
    libembttest_jni \
    factory.ini \
    libfwdlockengine \
    charger \
    charger_res_images \
    iw \
    tcpdump

ifneq ($(FOTA_UPDATE_SUPPORT),true)
PRODUCT_PACKAGES += \
    GoogleOta \
    GoogleOtaSysOper
endif

#for multimedia. set true to use ffmpeg; set other not to use.
ifeq ($(BOARD_USES_LIBPEONY),true)
PRODUCT_PACKAGES += \
    libpeony_demuxer \
    libavformat \
    libavutil \
    libavcodec \
    libswresample \
    libstagefright_soft_libpeony_ffmpeg_audio
endif

# HAL layer
PRODUCT_PACKAGES += \
    audio.primary.$(TARGET_PLATFORM) \
    lights.$(TARGET_PLATFORM) 	\
    sensors.$(TARGET_PLATFORM) \
    power.$(TARGET_PLATFORM)

RECOVERY_EXECUTABLES := \
    linker \
    factory \
    busybox \
    logcat \
    logwrapper \
    gsmMuxd \
    iw \
    healthd

# customer drivers
sinclude $(RDA_TARGET_DEVICE_DIR)/customer.mk
PRODUCT_PACKAGES += \
    rda_ts.ko \
    rda_gs.ko \
    rda_ls.ko \
    rda_headset.ko \
    rda_cam_sensor.ko

# build arm streamline tools
ifeq ($(TARGET_BUILD_VARIANT),eng)
PRODUCT_PACKAGES += \
    gator.ko \
    gatord
endif

BUILD_EXTMODULE := device/rda/common/extmodule.mk

# wifi bt firmware
PRODUCT_COPY_FILES += \
    device/rda/common/res/firmware/rda_wland/rda_wland.bin:vendor/firmware/rda_wland.bin \
    device/rda/common/res/firmware/rda_combo/rda_combo.bin:vendor/firmware/rda_combo.bin
    
PRODUCT_COPY_FILES += \
	device/rda/common/res/gms.tar.gz:vendor/lib/gms.tar.gz

ifneq ($(BUILD_WITHOUT_ANDROID), true)
ifeq "$(strip ${PLATFORM_VERSION})" "4.4.2"
PRODUCT_COPY_FILES += \
    device/rda/common/res/init.rc:root/init.rc \
    $(BOARDDIR)/init.$(TARGET_PLATFORM).rc:root/init.$(TARGET_PLATFORM).rc
else
PRODUCT_COPY_FILES += \
    device/rda/common/res/init_$(PLATFORM_VERSION).rc:root/init.rc \
    $(BOARDDIR)/init.$(TARGET_PLATFORM).rc:root/init.$(TARGET_PLATFORM).rc
endif
PRODUCT_COPY_FILES += \
    device/rda/common/res/init.ril_$(PLATFORM_VERSION).rc:root/init.ril.rc
else
PRODUCT_COPY_FILES += \
    device/rda/common/res/init.noandroid.rc:root/init.rc \
    device/rda/common/res/init.rda-noandroid.rc:root/init.$(TARGET_PLATFORM).rc
endif


PRODUCT_COPY_FILES += \
    $(BOARDDIR)/fstab:root/fstab \
    device/rda/common/res/init.rda.usb.rc:root/init.rda.usb.rc \
    $(BOARDDIR)/ueventd.$(TARGET_PLATFORM).rc:root/ueventd.$(TARGET_PLATFORM).rc \
    $(BOARDDIR)/init.recovery.$(TARGET_PLATFORM).rc:root/init.recovery.$(TARGET_PLATFORM).rc \
    $(BOARDDIR)/init.factory.rc:root/init.factory.rc \
    device/rda/common/res/bootlogo/initlogo_hvga.rle:root/initlogo.rle \
    $(RDA_TARGET_DEVICE_DIR)/oem_driver.rc:root/oem_driver.rc \
    device/rda/common/res/media/poweron.mp3:vendor/media/poweron.mp3 \
    device/rda/common/res/media/shutdown.mp3:vendor/media/shutdown.mp3 \
    device/rda/common/res/rda-keypad.kl:system/usr/keylayout/rda-keypad.kl \
    device/rda/common/res/apn/apns-conf.xml:system/etc/apns-conf.xml \
    device/rda/common/res/spn-conf.xml:system/etc/spn-conf.xml \
    device/rda/common/res/plmn-conf.xml:system/etc/plmn-conf.xml \
    device/rda/common/res/createswap.sh:system/xbin/createswap.sh \
    device/rda/common/res/mountcache.sh:system/xbin/mountcache.sh \
    device/rda/common/res/multidex.sh:system/xbin/multidex.sh \
    device/rda/common/res/unmountcache.sh:system/xbin/unmountcache.sh \
    device/rda/common/res/showswaps.sh:system/xbin/showswaps.sh \
    device/rda/common/res/gprs:system/etc/ppp/peers/gprs \
    device/rda/common/res/chat-gprs-connect:system/etc/ppp/chat-gprs-connect \
    device/rda/common/res/ip-up:system/etc/ppp/ip-up \
    device/rda/common/res/init.gprs-pppd:system/etc/ppp/init.gprs-pppd \
    device/rda/common/res/exit.gprs-pppd:system/etc/ppp/exit.gprs-pppd \
    device/rda/common/res/audio_policy.conf:system/etc/audio_policy.conf \
    device/rda/common/res/wallpaper/default_wallpaper.jpg:vendor/wallpaper/default_wallpaper.jpg \
    device/rda/common/res/wallpaper/default_wallpaper_small.jpg:vendor/wallpaper/default_wallpaper_small.jpg

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp \
    persist.sys.usb.serialno=1234567890ABCDEF

ifneq "$(strip ${PLATFORM_VERSION})" "4.4.2"
PRODUCT_COPY_FILES += \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml
endif

ifeq ($(BOARD_USES_SOFT_VCODECS),true)
PRODUCT_COPY_FILES += \
    $(RDA_TARGET_DEVICE_DIR)/camera_softenc.cfg:vendor/driver/camera.cfg \
    device/rda/common/res/media_profiles_soft.xml:system/etc/media_profiles.xml \
    device/rda/common/res/media_codecs_soft.xml:system/etc/media_codecs.xml

else ifeq ($(BOARD_USES_LIBPEONY),true)
PRODUCT_COPY_FILES += \
    $(RDA_TARGET_DEVICE_DIR)/camera.cfg:vendor/driver/camera.cfg \
    device/rda/common/res/media_profiles_extend.xml:system/etc/media_profiles.xml \
    device/rda/common/res/media_codecs_extend.xml:system/etc/media_codecs.xml

else
PRODUCT_COPY_FILES += \
    $(RDA_TARGET_DEVICE_DIR)/camera.cfg:vendor/driver/camera.cfg \
    device/rda/common/res/media_profiles.xml:system/etc/media_profiles.xml \
    device/rda/common/res/media_codecs.xml:system/etc/media_codecs.xml

endif

ifeq ($(TARGET_CUSTOMIZED_BOOTSHUTDOWN_ANIM),true)
# These are the boot and shutdown animation resources
PRODUCT_COPY_FILES += \
    $(BOARDDIR)/bootshutdownres/bootanimation.zip:vendor/media/bootanimation.zip \
    $(BOARDDIR)/bootshutdownres/shutanimation.zip:vendor/media/shutanimation.zip
endif

# These are the common hardware-specific features
PRODUCT_COPY_FILES += \
    vendor/3rdparty/misc/preinstallapk/preinstall.sh:/system/bin/preinstall.sh \
    device/rda/common/res/init.apanic.sh:/system/bin/init.apanic.sh \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \

# see $(TARGET_TABLET_MODE) in $(BOARDDIR)/$(TARGET_BOARD).mk
ifeq ($(TARGET_TABLET_MODE),true)
    PRODUCT_PROPERTY_OVERRIDES += ro.TabletMode=yes
    PRODUCT_COPY_FILES += \
        frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
        frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml
else
    PRODUCT_PROPERTY_OVERRIDES += ro.TabletMode=no
endif

ifneq ($(TARGET_GOOGLE_APP_LEVEL),none)
include device/rda/common/res/gms_pkg_$(PLATFORM_VERSION).mk
endif

ifeq ($(BOARD_SUPPORT_WIDEVINE_DRM),true)
PRODUCT_PACKAGES += \
	com.google.widevine.software.drm.xml \
	com.google.widevine.software.drm \
	libwvm \
	libdrmwvmplugin \
	libwvdrmengine \
	libdrmdecrypt
#	WidevineSamplePlayer
endif

ifeq ($(BOARD_SUPPORT_DRC),true)
PRODUCT_PACKAGES += libdrc
endif

ifeq ($(BOARD_SUPPORT_WIDEVINE_DRM),true)
PRODUCT_PROPERTY_OVERRIDES += \
    drm.service.enabled=true
endif

#fota start
ifeq ($(strip $(FOTA_UPDATE_SUPPORT)), true)
$(call inherit-product, vendor/3rdparty/adups-fota/FotaUpdate.mk)
endif

$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_no_telephony.mk)
