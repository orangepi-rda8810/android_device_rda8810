#
# Copyright (C) 2011 The Android Open-Source Project
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

# Build a separate vendor.img
TARGET_COPY_OUT_VENDOR := vendor


include device/rda/common/$(TARGET_BOARD_PLATFORM).mk
#config modem
TARGET_NO_MODEM := false
TARGET_PROVIDES_INIT_RC := true

# config gfx
ifeq "$(strip ${TARGET_BOARD_PLATFORM})" "rda8810"
BOARD_EGL_CFG := device/rda/common/res/egl.cfg
else
BOARD_EGL_CFG := vendor/3rdparty/arm/mali400-r4p0/driver/egl.cfg
endif
USE_OPENGL_RENDERER := true
# BUILD_EMULATOR_OPENGL := true

USE_CAMERA_STUB := false
BOARD_USES_GENERIC_AUDIO := true
#BOARD_USES_GENERIC_AUDIO := false
# Some framework code requires this to enable BT
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/generic/common/bluetooth
BOARD_HAVE_RDA_BLUETOOTH := true
#WLAN
#BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WLAN_DEVICE           := rdawfmac
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rdawfmac
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rdawfmac
WIFI_DRIVER_MODULE_PATH     := "/vendor/modules/rdawfmac.ko"
WIFI_DRIVER_MODULE_NAME     := "rdawfmac"

RDA_GOOGLEOTA_SUPPORT := true
RDA_FACTORY_MODE_SUPPORT := true
RDA_RECOVERY_MODE_SUPPORT := true

#config charger
BOARD_CHARGER_DISABLE_INIT_BLANK := true
BOARD_CHARGER_ENABLE_SUSPEND := true


#TARGET_DISABLE_TRIPLE_BUFFERING := true
# board specific modules
#BOARD_USES_ALSA_AUDIO := true
#BUILD_WITH_ALSA_UTILS := true
#BOARD_USE_VETH := true
#BOARD_USE_TD := true
#BOARD_USE_GSM := false
#HAVE_RADIO_IMG := false
