#!/system/bin/sh

#time=`date`
#echo start unmount shell at $time >>/data/test.txt
isodexonsdcard=`getprop ro.odex.on.sdcard 0`
iscachemounted=`df|grep /data/dalvik-cache-entry`
iscachemounting=`getprop app.dalvik-cache-dir.mounting false`
#echo iscachemounting $iscachemounting >>/data/test.txt
if [ -n "$iscachemounted" ] && [ "false" -eq $iscachemounting ]; then
        if [ "1" -eq $isodexonsdcard ];then
                setprop app.dalvik-cache-dir.unmounting true
                pids=`fuser -m /data/dalvik-cache-entry`
                #time=`date`
                #echo umount begin $time >>/data/test.txt
                #echo pids $pids >> /data/test.txt
                echo pids $pids
                retry=1
                while [[ -n "$pids" && $retry != 10 ]]
                do
                        #echo retry $retry >>/data/test.txt
                        echo retry $retry
                        pids=${pids// / }
                        for pid in $pids
                        do
                                #echo kill $pid >>/data/test.txt
                                echo kill $pid
                                kill $pid
                        done
                        sleep 1
                        pids=`fuser -m /data/dalvik-cache-entry`
                        let retry++
                done
                umount /data/dalvik-cache-entry
                #echo $? >>/data/test.txt
                #time=`date`
                #echo umount end $time >>/data/test.txt
                setprop app.replace.dalvik-cache-dir.d true
                setprop app.dalvik-cache-dir.unmounting false
        fi
fi
#time=`date`
#echo end unmount shell at $time >>/data/test.txt
