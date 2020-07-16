#!/system/bin/sh

iszram=`getprop ro.zram.swap.support 0`
isswapd=`getprop ro.disk.swap.support 0`
islowswap=`getprop ro.set.lowswap.support 0`
isodexonsdcard=`getprop ro.odex.on.sdcard 0`


b=`getprop sys.vm.swappiness 100`

if [ "1" -eq $iszram ];then
	a=`getprop zram.swap.disksize 128`
	
	echo $(($a*1024*1024)) > /sys/block/zram0/disksize
	mkswap /dev/block/zram0
	swapon /dev/block/zram0
	echo $(($b)) > /proc/sys/vm/swappiness
fi

if [ "1" -eq $isswapd ];then
	a=`getprop disk.swap.disksize 64`

	swapoff /dev/block/loop7;
	if [ ! -e /data/swap.img ];then
	    dd if=/dev/zero of=/data/swap.img  bs=1048576 count=$a;
	fi
	losetup /dev/block/loop7 /data/swap.img;
	mkswap /dev/block/loop7;
	swapon /dev/block/loop7;
	echo $(($b)) > /proc/sys/vm/swappiness
fi

if [ "1" -eq $islowswap ];then
	a=`getprop lowmemorykiller.lowswap.mfree 512`

	echo $(($a)) > /sys/module/lowmemorykiller/parameters/minsfree
fi

if [ "1" -eq $isodexonsdcard ];then
	setprop app.replace.dalvik-cache-dir.s true
	rm -rf /data/dalvik-cache
	rm -rf /data/local/tmp
	mkdir -p /data/dalvik-cache-entry
	ls /dev/block/mmcblk0
	if [ "0" -eq $? ];then
		rm -rf /data/dalvik-cache-entry/.dalvik-cache
		rm -rf /data/dalvik-cache-entry/.local_tmp
		mount -t vfat  -o nodev,dirsync,noexec,nosuid,utf8,uid=1023,gid=1023,fmask=000,dmask=000,shortname=mixed /dev/block/mmcblk0p1 /data/dalvik-cache-entry
		if [ "0" -ne $? ];then
			mount -t vfat  -o nodev,dirsync,noexec,nosuid,utf8,uid=1023,gid=1023,fmask=000,dmask=000,shortname=mixed /dev/block/mmcblk0 /data/dalvik-cache-entry
		fi
		size=`df | grep /data/dalvik-cache-entry | awk '{printf $4}'`
		ik=`expr index $size K`
		im=`expr index $size M`
		ig=`expr index $size G`
		if [ $ik -ne 0 ];then
			umount /data/dalvik-cache-entry
		fi
		if [ $im -ne 0 ];then
			size=${size%M*}
			if [ $size -lt 5 ];then
				umount /data/dalvik-cache-entry
			fi
		fi
	fi
	mkdir -p /data/dalvik-cache-entry/.dalvik-cache
	mkdir -p /data/dalvik-cache-entry/.local_tmp
	ln -s /data/dalvik-cache-entry/.dalvik-cache /data/dalvik-cache
	ln -s /data/dalvik-cache-entry/.local_tmp /data/local/tmp
	chmod 771 /data/dalvik-cache-entry
	chmod 771 /data/dalvik-cache-entry/.dalvik-cache
	chmod 771 /data/dalvik-cache
	chmod 771 /data/local/tmp
	chmod 771 /data/dalvik-cache-entry/.local_tmp
	chown system:system /data/dalvik-cache-entry/.dalvik-cache
	chown system:system /data/dalvik-cache-entry
	chown system:system /data/dalvik-cache
	mkdir -p /data/dalvik-cache/arm
	mkdir -p /data/dalvik-cache/profiles
	chown shell:shell /data/local/tmp
	chown shell:shell /data/dalvik-cache-entry/.local_tmp
	setprop app.replace.dalvik-cache-dir.d true
fi

