#!/system/bin/sh
a=`getprop zram.disksize 128`
b=`getprop sys.vm.swappiness 100`
c=`getprop sys.vm.min_free_kbytes 8`

echo $(($a*1024*1024)) > /sys/block/zram0/disksize
mkswap /dev/block/zram0
swapon /dev/block/zram0
echo $(($b)) > /proc/sys/vm/swappiness
#echo $(($c*1024)) > /proc/sys/vm/min_free_kbytes

