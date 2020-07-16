#!/system/bin/sh
needsformat=0
fonsize=`getprop ro.fon.size 32`

if [ ! -e /fat/fat.img ];then
    busybox dd if=/dev/zero of=/fat/fat.img  bs=1M seek=$fonsize count=0;
    needsformat=1
fi

losetup /dev/block/loop0 /fat/fat.img;

if [ "1" -eq $needsformat ];then
    mkfs.vfat /dev/block/loop0
fi

isodexonsdcard=`getprop ro.odex.on.sdcard 0`

if [ "2" -eq $isodexonsdcard ];then
	rm -rf /data/dalvik-cache
	mkdir -p /data/dalvik-cache-entry
	mount -t vfat  -o nodev,dirsync,noexec,nosuid,utf8,uid=1000,gid=1015,fmask=000,dmask=000,shortname=mixed /dev/block/loop0 /data/dalvik-cache-entry
	mkdir -p /data/dalvik-cache-entry/dalvik-cache
	ln -s /data/dalvik-cache-entry/dalvik-cache /data/dalvik-cache
	chmod 771 /data/dalvik-cache-entry
	chmod 771 /data/dalvik-cache-entry/dalvik-cache
	chmod 771 /data/dalvik-cache
	chown system:system /data/dalvik-cache-entry/dalvik-cache
	chown system:system /data/dalvik-cache-entry
	chown system:system /data/dalvik-cache
	setprop app.replace.dalvik-cache-dir true
fi

