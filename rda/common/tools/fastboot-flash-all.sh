#!/bin/sh

if [ -z "$ANDROID_PRODUCT_OUT" ]; then
	echo "Build env not setup"
	echo "Please go to top dir and run "
	echo "'. build/envsetup.sh'" 
	echo "'lunch'"
	exit 1
fi

fastboot flash bootloader $ANDROID_PRODUCT_OUT/bootloader.img
fastboot flash boot $ANDROID_PRODUCT_OUT/boot.img
fastboot flash modem $ANDROID_PRODUCT_OUT/modem.img
fastboot flash vendor $ANDROID_PRODUCT_OUT/vendor.img
fastboot flash recovery $ANDROID_PRODUCT_OUT/recovery.img
fastboot flash system $ANDROID_PRODUCT_OUT/system.img
