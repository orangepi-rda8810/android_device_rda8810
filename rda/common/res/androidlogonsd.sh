#!/system/bin/sh
create_dir="/sdcard/rdalog"
create_date=$(date +%Y%m%d%H%M%S)
mkdir -p $create_dir
logcat -f ${create_dir}/AndroidLog${create_date}.txt
