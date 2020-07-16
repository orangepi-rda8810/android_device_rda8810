#!/bin/bash

# This script is used to find all the libraries
# used by the executable files in factory mode

function usage() {
	ARGV0='$PRODUCT_OUT'
	ARGV1='$TARGET_RECOVERY_ROOT_OUT'
	ARGV2='$(TARGET_TOOLS_PREFIX)readelf'
	ARGV3='$(sort $(RECOVERY_SHARED_LIBRARIES))'
	echo "Usage: $0 $ARGV0 $ARGV1 $ARGV2 $ARGV3"
	exit 1
}

if [ "$#" -lt 3 ]; then
	usage
fi

ALL_SHARED_LIBRARIES=
PRODUCT_OUT=$1
TARGET_RECOVERY_ROOT_OUT=$2
TARGET_TOOLS_READELF=$3
shift 3

function find-needed-libs() {
	if [ "$1" == "bin" ]; then
		TARGET_ELF_FILE=$PRODUCT_OUT/obj/EXECUTABLES/$2_intermediates/$2
	else 
		TARGET_ELF_FILE=$PRODUCT_OUT/obj/SHARED_LIBRARIES/$2_intermediates/LINKED/$2.so

	fi

	$TARGET_TOOLS_READELF -a $TARGET_ELF_FILE | \
		awk '/NEEDED/{print $5}' | \
		sed -e 's/^\[//' -e 's/\]$//' -e 's/\.so//' | \
		xargs echo
}

function find-libs-recursive() {
	if [ "$1" == "bin" ]; then
		libraries_needed=$(find-needed-libs bin $2)
	else
		libraries_needed=$(find-needed-libs lib $2)
		if [[ " $ALL_SHARED_LIBRARIES " != *" $2 "* ]]; then
			ALL_SHARED_LIBRARIES+=" $2"
		fi
	fi

	if [ -n "$libraries_needed" ]; then
		for library in $libraries_needed; do
			find-libs-recursive lib $library
		done
	fi
}

for bin in $@; do
	find-libs-recursive bin $bin
done

echo $ALL_SHARED_LIBRARIES
