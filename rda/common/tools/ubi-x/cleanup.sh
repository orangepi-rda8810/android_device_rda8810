#!/bin/bash

sudo umount /dev/ubi0_0
sudo rmmod ubifs ubi nandsim

rm -rf system vendor nand_cache.*

