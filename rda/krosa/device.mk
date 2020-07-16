#
# Copyright (C) 2009 The Android Open Source Project
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

# This is a build configuration for a full-featured build of the
# Open-Source part of the tree. It's geared toward a US-centric
# build of the emulator, but all those aspects can be overridden
# in inherited configurations.
include $(ANDROID_BUILD_TOP)/$(RDA_TARGET_DEVICE_DIR)/target.mk

#BUILD_WITHOUT_ANDROID := true

PRODUCT_PROPERTY_OVERRIDES := \
    keyguard.no_require_sim=true \
    ro.com.android.dataroaming=true \
    ro.config.low_ram=true \
    ro.config.max_starting_bg=8 \

ifeq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_PROPERTY_OVERRIDES += \
    ro.adb.secure=1
endif

ifneq "$(strip ${PLATFORM_VERSION})" "4.4.2"
PRODUCT_PROPERTY_OVERRIDES += \
     dalvik.vm.dex2oat-filter=verify-none \
     dalvik.vm.image-dex2oat-filter=speed
PRODUCT_DEX_PREOPT_DEFAULT_FLAGS := --compiler-filter=verify-none
PRODUCT_COPY_FILES += \
    device/rda/common/res/init.storage.rc:root/init.storage.rc
endif

# config components
ifeq "$(strip ${TARGET_BOARD_PLATFORM})" "rda8810"
BOARD_USES_GPU_VIVANTE := true
endif
ifeq "$(strip ${TARGET_BOARD_PLATFORM})" "rda8810e"
BOARD_USES_GPU_MALI := true
endif
ifeq "$(strip ${TARGET_BOARD_PLATFORM})" "rda8850e"
BOARD_USES_GPU_MALI := true
BOARD_USES_JPU_CODA := true
endif
ifeq "$(strip ${TARGET_BOARD_PLATFORM})" "rda8810h"
BOARD_USES_GPU_MALI := true
endif
BOARD_USES_VPU_CODA := true
#BOARD_USES_LIBPEONY := true
#switch hardware or soft video codecs.sheen
#BOARD_USES_SOFT_VCODECS := true
BOARD_USES_VOC := true
BOARD_USES_CAM := true

Customer_PACKAGES :=
# customer borad config
sinclude $(BOARDDIR)/$(TARGET_HARDWARE_CFG)/customer_config.mk

App_PACKAGES := \
    SoundRecorder \
    RdaFMRadio \
    FMRadio \
    SchedulePowerOnOff \
    librdafm \
    PicoTts \
    FileManager \
    UserDictionaryProvider \
    CallFireWall \
    com.rda.firewall \
    AppTransfer \
    CellBroadcastReceiver \

ifeq "$(strip ${PLATFORM_VERSION})" "4.4.2"
App_PACKAGES += \
    modifyVersionXml
endif

ifneq "$(strip ${CUSTOMER_DISABLE_ROOT})" "yes"
App_PACKAGES += \
    su \
    Superuser
endif

#    MatvPlayer \
#    librdamtv_jni \
#    WAPPushManager \
#    UserDictionaryProvider \
#    XiaoMi \
#    HTMLViewer \
#    VoiceDialer \
#    FileExplorer \
#    VideoPlayer \
#    FileManager \
#    CellBroadcastReceiver \
#    BasicSmsReceiver \
#    PicoTts \

LiveWallpaper_PACKAGES := \
    LiveWallpapers \
    Galaxy4 \
    HoloSpiralWallpaper \
    LiveWallpapersPicker \
    MagicSmokeWallpapers \
    VisualizationWallpapers \
    NoiseField \
    PhaseBeam \
    VisualizationWallpapers \
    PhotoTable \
    BasicDreams \

InputMethod_PACAGES := \
    LatinIME \

#    PinyinIME \
#    libjni_pinyinime \

#latin inputmethod
#    LatinIME \

#i18n inputmethod
#    Komoxo \
#    libKomoxo \

#japaneses
#    OpenWnn \
#    libWnnEngDic \
#    libWnnJpnDic \
#    libwnndict

PRODUCT_PACKAGES := \
    $(App_PACKAGES) \
    $(InputMethod_PACAGES) \
    $(LiveWallpaper_PACKAGES) \
    $(Customer_PACKAGES)

PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*.jpg,device/rda/common/res/wallpaper,vendor/wallpaper) \

# Additional settings used in all AOSP builds
PRODUCT_PROPERTY_OVERRIDES += \
    ro.com.android.dateformat=MM-dd-yyyy \
    ro.config.ringtone0=Ring_Synth_04.ogg \
    ro.config.ringtone1=Ring_Synth_04.ogg \
    ro.config.notification_sound=pixiedust.ogg

# Setting default font size
#  small:0.85f, normal:1.15f, large:1.20f, huge:1.30f
PRODUCT_PROPERTY_OVERRIDES += \
    ro.property.fontScale=1.20f

# Include drawables for all densities
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi xxhdpi

# Get some sounds
$(call inherit-product-if-exists, frameworks/base/data/sounds/AllAudio.mk)

# Get the TTS language packs
ifeq ($(findstring PicoTts,$(App_PACKAGES)),PicoTts)
$(call inherit-product-if-exists, external/svox/pico/lang/all_pico_languages.mk)
endif

# Get a list of languages.
$(call inherit-product, $(SRC_TARGET_DIR)/product/locales_full.mk)

PRODUCT_LOCALES := zh_CN en_US
#$(call inherit-product, build/target/product/languages_full.mk)
#APPLICATION and other properties
ifneq ($(strip $(CUSTOMER_PACKAGE_OVERLAYS)),)
DEVICE_PACKAGE_OVERLAYS := $(CUSTOMER_PACKAGE_OVERLAYS)
else
DEVICE_PACKAGE_OVERLAYS := $(BOARDDIR)/overlay
endif

ifneq "$(strip ${PLATFORM_VERSION})" "4.4.2"
DEVICE_PACKAGE_OVERLAYS := $(DEVICE_PACKAGE_OVERLAYS)_$(PLATFORM_VERSION)
endif

# Call RDA common makefiles
$(call inherit-product, device/rda/common/base.mk)
$(call inherit-product, device/rda/common/proprietories.mk)
