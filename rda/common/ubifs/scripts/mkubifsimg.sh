#!/bin/bash

# To Generate UBI image.
#
# Need 8 Params
# $1 - PAGE SIZE of nand (bytes) - e.g. 2048, 4096, ...
# $2 - PAGE NUMBER OF ONE BLOCK of nand - e.g. 64
# $3 - PARTITION SIZE of mtd to write this image (bytes) - e.g. 134217728 (128 MiB)
# $4 - MAX TOLERATE BAD BLOCK NUMBER - e.g. when this mtd partition has 20 bad blocks, product can NOT work anymore.
# $5 - INPUT DIR to gen this image - e.g. out/target/product/aere/system
# $6 - OUTPUT NAME of image - e.g. system.img
# $7 - VOLUME NAME when mount - e.g. system
# $8 - COMPRESSION TYPE - e.g. zlib, lzo
# $9 - IMAGE TYPE - e.g. ubi(for mtd part), ubifs(for ubi volume)

check_result() {
if [ $? -ne 0 ]
then
	echo -en "Fail. \n"
	exit -1
else
	echo -en "Success. \n"
fi
}

if [ $# -lt 8 ]
then
	echo -en "usage:
	mkubifsimg.sh \n
	#1 - PAGE SIZE of nand (bytes) - e.g. 2048, 4096, ... \n
	#2 - PAGE NUMBER OF ONE BLOCK of nand - e.g. 64 \n
	#3 - PARTITION SIZE of mtd to write this image (bytes) - e.g. 134217728 (128 MiB) \n
	#4 - MAX TOLERATE BAD BLOCK NUMBER PERCENT - e.g. when this mtd partition has 6% bad blocks, product can NOT work anymore. \n
	#5 - INPUT DIR to gen this image - e.g. out/target/product/aere/system \n
	#6 - OUTPUT NAME of image - e.g. system.img \n
	#7 - VOLUME NAME when mount - e.g. system \n
	#8 - COMPRESSION TYPE - e.g. zlib, lzo \n
	#9 - IMAGE TYPE - e.g. ubi(for mtd part), ubifs(for ubi volume)
	#10 - selinux file context file

	e.g. : \n
	  mkubifsimg.sh 2048 64 134217728 20 out/target/product/aere/system/ out/target/product/aere/system.img system zlib ubi\n
	your cmd ($# args) is : \n
	  $*\n"
	exit
fi

### params
physical_page_size=$1
echo "PHYSICAL PAGE SIZE                    --- ($physical_page_size) (Bytes)"
page_number_per_physical_block=$2
echo "PAGE NUMBER PER PHYSICAL BLOCK        --- ($page_number_per_physical_block)"
physical_partition_size=$3
echo "PHYSICAL PARTITION SIZE               --- ($physical_partition_size) (Bytes)"
max_tolerate_bad_block_number_percent=$4
echo "MAX TOLERATE BAD BLOCK NUMBER PERCENT --- ($max_tolerate_bad_block_number_percent%)"
input_dir=$5
echo "INPUT DIR                             --- ($input_dir)"
output_name=$6
echo "OUTPUT NAME                           --- ($output_name)"
volume_name=$7
echo "VOLUME NAME                           --- ($volume_name)"
compr_name=$8
echo "COMPRESSION NAME                      --- ($compr_name)"
image_type=$9
echo "IMAGE TYPE                            --- ($image_type)"
if [ ! -z ${10} ]
then
fc_context=${10}
else
fc_context=""
fi
if [ ! -z $fc_context ]
then
echo "Selinux FILE CONTEXT                  --- ($fc_context)"
fi

### calculate
# PHYSICAL BLOCK SIZE = PHYSICAL PAGE SIZE * PAGE NUMBER PER PHYSICAL BLOCK
physical_block_size=`expr $physical_page_size \* $page_number_per_physical_block`
echo "PHYSICAL BLOCK SIZE                   --- ($physical_block_size) (Bytes)"

# PHYSICAL BLOCK NUMBER OF PARTITION = PHYSICAL PARTITION SIZE / PHYSICAL BLOCK SIZE
physical_block_number_of_this_partition=`expr $physical_partition_size / $physical_block_size`
echo "PHYSICAL BLOCK NUMBER OF PARTITION    --- ($physical_block_number_of_this_partition)"

max_tolerate_bad_block_number=`expr $physical_block_number_of_this_partition \* $max_tolerate_bad_block_number_percent / 100`
if [ $max_tolerate_bad_block_number -lt 6 ]
then
    let max_tolerate_bad_block_number=6
fi
echo "MAX TOLERATE BAD BLOCK NUMBER         --- ($max_tolerate_bad_block_number)"

# BAD BLOCK HANDLE RESERVED BLOCKS is 1% OF TOTAL BLOCKS : kernel config
bad_block_handle_reserved_blocks=`expr $physical_block_number_of_this_partition / 100`
if [ $bad_block_handle_reserved_blocks -lt 2 ]
then
	let bad_block_handle_reserved_blocks=2
fi
echo "RESERVED BLOCKS FOR BAD BLOCK HANDLE  --- ($bad_block_handle_reserved_blocks)"

# WEAR LEVEL RESERVED BLOCKS : 1 defined in kernel/driver/mtd/ubi/wl.c
wl_reserved_blocks=1
echo "RESERVED BLOCKS FOR WEAR LEVEL        --- ($wl_reserved_blocks)"

# EBA RESERVED BLOCKS : 1 defined in kernel/driver/mtd/ubi/eba.c
eba_reserved_blocks=1
echo "RESERVED BLOCKS FOR EBA               --- ($eba_reserved_blocks)"

# LAYOUT VOLUME RESERVED BLOCKS : layout voluem use 2 blocks
layout_volume_reserved_blocks=2
echo "RESERVED BLOCKS FOR UBI LAYOUT VOLUME --- ($layout_volume_reserved_blocks)"

# PAGE NUMBER PER LOGICAL ERASE BLOCK = PAGE NUMBER PER PHYSICAL BLOCK - 2
page_number_per_logical_erase_block=`expr $page_number_per_physical_block - 2`
echo "PAGE NUMBER PER LOGICAL ERASE BLOCK   --- ($page_number_per_logical_erase_block)"

# LOGICAL ERASE BLOCK SIZE = PAGE NUMBER OF LOGICAL ERASE BLOCK * PHYSICAL PAGE SIZE
logical_erase_block_size=`expr $physical_page_size \* $page_number_per_logical_erase_block`
echo "LOGICAL ERASE BLOCK SIZE              --- ($logical_erase_block_size) (Bytes)"

# LOGICAL BLOCKS ON PARTITION = PHYSICAL BLOCK NUMBER OF PA - RESERVED BLOCKS (wear level , bad block ...)
ubi_reserved_blocks=`expr $bad_block_handle_reserved_blocks + $wl_reserved_blocks`
ubi_reserved_blocks=`expr $ubi_reserved_blocks + $eba_reserved_blocks`
ubi_reserved_blocks=`expr $ubi_reserved_blocks + $layout_volume_reserved_blocks`
logical_block_number_of_this_partition=`expr $physical_block_number_of_this_partition - $ubi_reserved_blocks`
logical_block_number_of_this_partition=`expr $logical_block_number_of_this_partition  - $max_tolerate_bad_block_number`
echo "LOGICAL BLOCK NUMBER OF PARTITION     --- ($logical_block_number_of_this_partition)"

# FILE SYSTEM VOLUEM SIZE = LOGICAL BLOCK NUMBER * LOGICAL ERASE BLOCK SIZE
fs_vol_size=`expr $logical_block_number_of_this_partition \* $logical_erase_block_size`
echo "FILE SYSTEM VOLUME SIZE               --- ($fs_vol_size) (Bytes)"


ubifs_img_file=`mktemp -u`.ubifs_img_file.$volume_name
ubinize_cfg_file=`mktemp -u`.ubinize_cfg_file.$volume_name

clean() {
	mkdir -p /tmp/create-ubi-image/
	rm -rf $ubifs_img_file $ubinize_cfg_file
}

clean

echo
echo "Generating configuration file..."
echo "[ubi-image]"  > $ubinize_cfg_file
echo "mode=ubi" >> $ubinize_cfg_file
echo "image=$ubifs_img_file" >> $ubinize_cfg_file
echo "vol_id=0" >> $ubinize_cfg_file
echo "vol_size=$fs_vol_size" >> $ubinize_cfg_file
echo "vol_type=dynamic" >> $ubinize_cfg_file
echo "vol_name=$volume_name" >> $ubinize_cfg_file
echo "Done"

echo
echo -en "Generating ubifs image ...\n"

if [ ! -z $fc_context ];
then
echo "SELINUX FC is $fc_context "
mkubifs \
	-m $physical_page_size \
	-e $logical_erase_block_size \
	-c $logical_block_number_of_this_partition \
	-r $input_dir \
	-o $ubifs_img_file \
	-x $compr_name \
	-F \
	-v \
	--selinux=$fc_context
else
mkubifs \
	-m $physical_page_size \
	-e $logical_erase_block_size \
	-c $logical_block_number_of_this_partition \
	-r $input_dir \
	-o $ubifs_img_file \
	-x $compr_name \
	-F \
	-v
fi

# check partition free block number, if it is over ${max_free_block_percent}, print warning
max_free_block_percent=40
suggest_free_block_percent=15
ubifs_img_size=`du -b $ubifs_img_file | awk '{print $1}'`
used_block_number=`expr $ubifs_img_size / $logical_erase_block_size + 1`
free_block_number=`expr $logical_block_number_of_this_partition - $used_block_number`
free_block_percent=`expr $free_block_number \* 100 / $physical_block_number_of_this_partition`
if [ $free_block_percent -gt $max_free_block_percent ];
then
	reserved_and_used_blocks=`expr $used_block_number + $ubi_reserved_blocks`
	suggest_reserve_and_used_block_percent=`expr 100 - $suggest_free_block_percent - $max_tolerate_bad_block_number_percent`
	suggest_physical_block_number=`expr $reserved_and_used_blocks \* 100 / $suggest_reserve_and_used_block_percent + 1`
	suggest_partition_size=`expr $suggest_physical_block_number \* $physical_block_size`
	suggest_partition_size_m=`expr $suggest_partition_size / 1024 / 1024`
	physical_partition_size_m=`expr $physical_partition_size / 1024 / 1024`
	echo
	echo "##########################################################################################"
	echo "##########################################################################################"
	echo "## WARNING:   Part [${volume_name}], free space is ${free_block_percent}%, OVER ${max_free_block_percent}%."
	echo "## SUGGETION: Part [${volume_name}], adjust part size, ${physical_partition_size_m}M -> ${suggest_partition_size_m}M."
	echo "##########################################################################################"
	echo "##########################################################################################"
	echo
	exit -1
fi

check_result
if [ $image_type = "ubifs" ];
then
	/bin/cp -v $ubifs_img_file $output_name
else
	echo -en "Generating ubi image ...\n"
	ubinize \
		-m $physical_page_size \
		-p $physical_block_size \
		$ubinize_cfg_file \
		-o $output_name \
		-v
	check_result
fi

clean
echo -en "Done.\n"
