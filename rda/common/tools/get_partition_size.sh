#!/bin/bash

part=$1
#echo 'get mtdpart' $part size>&2
mtdtab_path=$ANDROID_BUILD_TOP/$RDA_TARGET_DEVICE_DIR/include/tgt_ap_flash_parts.h
part_info=`grep \($part\) $mtdtab_path`
part_info=${part_info%\\*}
#echo "the partinfo" $part_info>&2
if [ -z $part_info ];then
	echo "cannot find the part">&2
	exit -1
fi

#### for partion
#echo "get partion " $part "'s info">&2
part_info=${part_info%%\($part\)*}
part_size=${part_info#*\"}
index=`expr index $part_size K`
if [ $index  -ne 0 ];then
#	echo "there is Kilo" $index
	part_size=${part_size%K*}
#	echo "partion size" $part_size Kilo
	part_size=`expr $part_size \* 1024`
	#echo $part_size
	#exit 0
fi
index=`expr index $part_size M`
if [ $index -ne 0 ];then
#	echo "there is Mega" $index
	part_size=${part_size%M*}
#	echo "partion size" $part_size Miga
	part_size=`expr $part_size \* 1024 \* 1024`
	#echo $part_size
	#exit 0
fi
index=`expr index $part_size G`
if [ $index -ne 0 ];then
#	echo "there is Giga" $index
	part_size=${part_size%G*}
#	echo "partion size" $part_size Giga
	part_size=`expr $part_size \* 1024 \* 1024 \* 1024`
	#echo $part_size
	#exit 0
fi

#### for partion reserved
part_r_info=`grep \(${part}_reserved_size\) $mtdtab_path`
part_r_info=${part_r_info%\\*}

#echo "the partinfo" $part_r_info>&2
if [ -z $part_r_info ];then
	# echo "cannot find the part reserved ">&2
	# exit -1
	echo $part_size
	exit 0
fi
part_r_info=${part_r_info%%\(${part}_reserved_size\)*}
part_r_size=${part_r_info#*\"}
index=`expr index $part_r_size K`
if [ $index  -ne 0 ];then
#	echo "there is Kilo" $index
	part_r_size=${part_r_size%K*}
#	echo "partion size" $part_r_size Kilo
	part_r_size=`expr $part_r_size \* 1024`
	#echo $part_r_size
	#exit 0
fi
index=`expr index $part_r_size M`
if [ $index -ne 0 ];then
#	echo "there is Mega" $index
	part_r_size=${part_r_size%M*}
#	echo "partion size" $part_r_size Miga
	part_r_size=`expr $part_r_size \* 1024 \* 1024`
	#echo $part_r_size
	#exit 0
fi
index=`expr index $part_r_size G`
if [ $index -ne 0 ];then
#	echo "there is Giga" $index
	part_r_size=${part_r_size%G*}
#	echo "partion size" $part_r_size Giga
	part_r_size=`expr $part_r_size \* 1024 \* 1024 \* 1024`
	#echo $part_r_size
	#exit 0
fi

let part_size=$part_size-$part_r_size
echo $part_size
