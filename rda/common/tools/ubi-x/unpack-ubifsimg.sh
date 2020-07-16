#!/bin/bash

function print_usage()
{
cat <<EOF
useage:
unpack_ubifsimg.sh system.img/vendor.img
EOF
}

function get_part_value()
{
	local index

	index=${#1}
	index=`expr $index \+ 1`
	#echo "index " $index
	while read line
	do
	#	echo "line" $line
		if [ `expr $line : $1` -gt 0 ];
		then :
			echo ${line:$index} 
			break	
		fi
	done < ubimg.cfg
}

function get_pagesz
{
	local size

	size=$(get_part_value PAGESIZE)
	echo $size
}

if [ "$1" ]; then :
	image=$1
else
	echo "fatal ERROR:"
	print_usage
	exit -1
fi

if [ $image != "system.img" ] && [ $image != "vendor.img" ];
then
	print_usage
	exit -1
fi

#get system or cusomer image name
temp_name=${image//./ }
temp_arr=($temp_name)
image_dir=${temp_arr[0]}
echo "image name" $image_dir

if [ -d $image_dir ]
then :
	echo "ERROR:"
	echo "the old path" $image_dir "is still exist"
	echo "please run cleanup.sh "
	exit -1
fi
mkdir -p $image_dir

nandpage_sz=$(get_pagesz)

echo "nand page size:(2048/4096):" $nandpage_sz

if [ $nandpage_sz = "2048" ]; then :
	nandsim_args="first_id_byte=0xec second_id_byte=0xd3 third_id_byte=0x51\
		fourth_id_byte=0x95 cache_file=nand_cache.$image_dir"
elif [ $nandpage_sz = "4096" ]; then :
	nandsim_args="first_id_byte=0xec second_id_byte=0xd5 third_id_byte=0x51\
		fourth_id_byte=0xa6 cache_file=nand_cache.$image_dir"
else
	echo "dont supprot this page size" nandpage_sz
fi

echo $cache_params

echo "install module nand simulator"
sudo modprobe nandsim $nandsim_args

echo "copy the" $image "to the nand simulator"
sudo dd if=$image of=/dev/mtd0 bs=$nandpage_sz

echo "attach ubi to the nand simulator"
sudo modprobe ubi mtd=0,$nandpage_sz

echo "mount the ubifs to the direcory" $image_dir
sudo mount -t ubifs /dev/ubi0_0 $image_dir
