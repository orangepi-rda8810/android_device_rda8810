#!/system/bin/sh

#time=`date`
#echo start mount shell at $time >>/data/test.txt
isodexonsdcard=`getprop ro.odex.on.sdcard 0`

iscacheunmounting=`getprop app.dalvik-cache-dir.unmounting false`
while [[ "true" = "$iscacheunmounting" ]]
do
        #echo waite unmount end iscacheunmounting $iscacheunmounting >>/data/test.txt
        sleep 1
        iscacheunmounting=`getprop app.dalvik-cache-dir.unmounting false`
done
iscachemounted=`df | grep /data/dalvik-cache-entry`
#echo iscachemounted $iscachemounted >>/data/test.txt
if [ "1" -eq $isodexonsdcard ] && [ -z "$iscachemounted" ];then
        setprop app.dalvik-cache-dir.mounting true
        #mountdate=`date`
        #echo start mount at $mountdate >>/data/test.txt
	rm -rf /data/dalvik-cache
	rm -rf /data/local/tmp
	mkdir -p /data/dalvik-cache-entry
	ls /dev/block/mmcblk0
	if [ "0" -eq $? ];then
                #echo got mmcblk0 >>/data/test.txt
		rm -rf /data/dalvik-cache-entry/.dalvik-cache
		rm -rf /data/dalvik-cache-entry/.local_tmp
		mount -t vfat  -o nodev,dirsync,noexec,nosuid,utf8,uid=1023,gid=1023,fmask=000,dmask=000,shortname=mixed /dev/block/mmcblk0p1 /data/dalvik-cache-entry
		if [ "0" -ne $? ];then
			mount -t vfat  -o nodev,dirsync,noexec,nosuid,utf8,uid=1023,gid=1023,fmask=000,dmask=000,shortname=mixed /dev/block/mmcblk0 /data/dalvik-cache-entry
		fi
		size=`df | grep /data/dalvik-cache-entry | awk '{printf $4}'`
                #echo size $size >>/data/test.txt
		ik=`expr index $size K`
		im=`expr index $size M`
		ig=`expr index $size G`
		if [ $ik -ne 0 ];then
                        #echo do unmount in mountcache.sh >>/data/test.txt
			umount /data/dalvik-cache-entry
		fi
		if [ $im -ne 0 ];then
			size=${size%M*}
			if [ $size -lt 5 ];then
                                #echo do unmount in mountcache.sh 2 >>/data/test.txt
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
	chown shell:shell /data/local/tmp
	chown shell:shell /data/dalvik-cache-entry/.local_tmp
	setprop app.replace.dalvik-cache-dir.d true
        #mountdate=`date`
        #echo end mount at $mountdate >>/data/test.txt
        setprop app.dalvik-cache-dir.mounting false
fi
#time=`date`
#echo end mount shell at $time >>/data/test.txt

