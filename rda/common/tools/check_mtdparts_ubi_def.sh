#!/bin/bash

mtdtab_path=$ANDROID_BUILD_TOP/$RDA_TARGET_DEVICE_DIR/include/tgt_ap_flash_parts.h
info=`grep MTDPARTS_UBI_DEF $mtdtab_path`
if [ -z "$info" ];
then
	echo "no"
else
	echo "yes"
fi
exit 0

