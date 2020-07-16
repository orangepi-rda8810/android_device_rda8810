#!/bin/sh

if [ -z $1 ]; then
	echo "Usage: mkinitlogo.sh xxx.png"
	exit 1;
fi

which convert > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "ERROR command convert doesn't exist, please run:"
	echo "sudo apt-get install imagemagick"
	exit 1;
fi

which rgb2565 > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "ERROR command rgb2565 doesn't exist, please build android first"
	exit 1;
fi

echo "covert $1 to initlogo..."
convert -depth 8 $1 rgb:temp.raw
rgb2565 -rle < temp.raw >initlogo.rle
rm temp.raw
