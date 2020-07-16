#!/bin/bash

function print_usage()
{
cat <<EOF
useage:
pack_ubifsimg.sh system/vendor
EOF
}

function get_part_value()
{
	local index

	index=${#1}
	#ignore the "="
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

function get_blocksz
{
	local blk_size

	blk_size=$(get_part_value BLOCKSIZE)
	echo $blk_size
}

function get_partsz
{
	local part_size

	part_size=$(get_part_value $1)
	echo $part_size
}

if [ "$1" ]; then :
	image=$1
else
	echo "fatal ERROR:"
	print_usage
	exit -1
fi

if [ $image != "system" ] && [ $image != "vendor" ];
then
	print_usage
	exit -1
fi

pagesize=$(get_pagesz)
echo "flash page size" $pagesize

blocksize=$(get_blocksz)
echo "flash block size" $blocksize

if [ $image == "system" ];
then
	partsize=$(get_partsz system.partsize)
elif [ $image == "vendor" ];
then
	partsize=$(get_partsz vendor.partsize)
fi
echo $image "part size" $partsize

pagenum=`expr $blocksize \/ $pagesize`
echo "pagenum/block is" $pagenum

echo "change the orignal owner"
sudo chown `whoami`:`whoami` -R $image/

echo "repack the image to the $image-new.img"
PATH=./:$PATH
./mkubifsimg.sh $pagesize $pagenum $partsize 6 $image/ $image-new.img $image zlib ubi

