#!/bin/sh
if [ ! "$ANDROID_PRODUCT_OUT" ]; then
	echo "Error: Please select lunch and target"
   	exit -1
fi

total_bin=$ANDROID_PRODUCT_OUT/rda_${TARGET_HARDWARE_CFG}_total.bin
rm -f $total_bin
touch $total_bin

declare -a image_list
index=0
offset=0
image_cnt=0

while read line
do
	if [ ${line:0:1} == "#" ] #this is commented
	then :
		continue
	fi
	echo 
	image_attr=($line)
	image_name=${image_attr[0]}
	image_list[$index]=$ANDROID_PRODUCT_OUT/$image_name
	download_name=${image_attr[1]}
	packet_size=${image_attr[2]}
	address=${image_attr[3]}
	executable=${image_attr[4]}
	echo "image name" $image_name
	echo "download name" $download_name
	echo "packet size" $packet_size
	echo "address" $address
	echo "executable" $executable
	image_size=`stat -c "%s" ${image_list[$index]}`
	echo "image size" $image_size
	echo "offset" $offset
	image_cnt=`expr $index \+ 1`
	echo "image counts" $image_cnt

	build_header -f $total_bin -s $image_size -i $download_name -o $offset\
		-a $address -p $packet_size -x $executable -c $image_cnt

	offset=`expr $image_size \+ $offset`
	index=`expr $index \+ 1`
done < product.cfg

#write the whole images
echo
echo "write image to" $total_bin
index=0;
while [ $index -lt $image_cnt ]
do
	cat ${image_list[$index]} >> $total_bin
	index=$[$index + 1]
done
