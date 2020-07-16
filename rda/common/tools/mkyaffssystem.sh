#!/bin/sh

if [ -z "$ANDROID_PRODUCT_OUT" ]; then
	echo "Build env not setup"
	echo "Please go to top dir and run "
	echo "'. build/envsetup.sh'" 
	echo "'lunch'"
	exit 1
fi

mkyaffs2image -f $ANDROID_PRODUCT_OUT/system $ANDROID_PRODUCT_OUT/system.yaffs
mkyaffs2image -f $ANDROID_PRODUCT_OUT/data $ANDROID_PRODUCT_OUT/userdata.yaffs
