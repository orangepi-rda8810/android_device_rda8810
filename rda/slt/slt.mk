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

#this file is included by u-boot make file, that is why we need
#to use abs path here
include $(ANDROID_BUILD_TOP)/$(RDA_TARGET_DEVICE_DIR)/target.mk


TARGET_PLATFORM := $(TARGET_BOARD_PLATFORM)
TARGET_BOARD := slt
BOARDDIR := device/rda/$(TARGET_BOARD)

# tablet mode
TARGET_TABLET_MODE := false

# use customized boot and shutdown animation
TARGET_CUSTOMIZED_BOOTSHUTDOWN_ANIM := false

# enable internal storage by android's emulated method
TARGET_INTERNAL_STORAGE_EMULATED := false

# enable internal storage by virtual FAT partition on nand
TARGET_FAT_ON_NAND := false

# enable internal storage by FAT partition with NFTL
TARGET_FAT_NFTL := false

ifeq "$(strip ${PLATFORM_VERSION})" "4.4.2"
# fs_type
# TARGET_SYSTEMIMAGE_USE_YAFFS2 := true
# TARGET_SYSTEMIMAGE_USE_SQUASHFS := true
# TARGET_USERIMAGES_USE_YAFFS2 := true
# TARGET_SYSTEMIMAGE_USE_UBIFS := true
# TARGET_USERIMAGES_USE_UBIFS := true
TARGET_SYSTEMIMAGE_USE_EXT4 := true
TARGET_USERIMAGES_USE_EXT4 := true

# nand flash config
# BOARD_NAND_YAFFS2_TAGS_INBAND := true
# TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true
else
# fs_type: ubifs,squashfs,ext4
#BOARD_SQUASHFS_COMPRESSOR := xz
#BOARD_SQUASHFS_COMPRESSOR_OPT := -Xbcj arm
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
TARGET_USERIMAGES_USE_EXT4 := true
endif

# Enable dex-preoptimization to speed up the first boot sequence
# Open this on compressed file system is very useful
# Turn ON this if SQUASHFS is used, otherwise, turn this OFF
# Since UBIFS has compression too, DEXPREOPT is ON for UBIFS
WITH_DEXPREOPT := true
WITH_DEXPREOPT_PIC := true

# include classified configs
$(call inherit-product, $(BOARDDIR)/device.mk)

# Overrides
ifeq "$(strip ${PLATFORM_VERSION})" "4.4.2"
PRODUCT_RUNTIMES := runtime_libdvm_default
endif
PRODUCT_NAME := $(TARGET_BOARD)
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := RDA SmartPhone
PRODUCT_BRAND := RDA
PRODUCT_MANUFACTURER := Rdamicro
