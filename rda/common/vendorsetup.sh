#
# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

########################################################################
# Quick commands for Android Building
########################################################################

# a quick command to install kernel header files for userspace building
function kheader()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	mkdir -p $OUT/obj/KERNEL
	make -C $T/kernel O=$OUT/obj/KERNEL ARCH=arm CROSS_COMPILE=arm-eabi- headers_install
}

# a quick command to launch kernel menuconfig
function kmconfig()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	mkdir -p $OUT/obj/KERNEL
	make -C $T/kernel O=$OUT/obj/KERNEL ARCH=arm CROSS_COMPILE=arm-eabi- menuconfig
}

# a quick command to launch kernel menuconfig and update .config to kernel
function kuconfig()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	KERNEL_DEFCONFIG=$(get_build_var KERNEL_DEFCONFIG)
	mkdir -p $OUT/obj/KERNEL
	make -C $T/kernel O=$OUT/obj/KERNEL ARCH=arm CROSS_COMPILE=arm-eabi- menuconfig
	cp $OUT/obj/KERNEL/.config $T/kernel/arch/arm/configs/$KERNEL_DEFCONFIG
}

# a quick command to update kernel config to the corresponding defconfig
function kdconfig()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	KERNEL_DEFCONFIG=$(get_build_var KERNEL_DEFCONFIG)
	mkdir -p $OUT/obj/KERNEL
	make -C $T/kernel O=$OUT/obj/KERNEL ARCH=arm CROSS_COMPILE=arm-eabi- $KERNEL_DEFCONFIG
}

# a quick command to clean kernel building files
function kclean()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	make -C $T/kernel O=$OUT/obj/KERNEL ARCH=arm CROSS_COMPILE=arm-eabi- mrproper
}

# a quick command to run kernel make
function kmk()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	make -C $T/kernel O=$OUT/obj/KERNEL ARCH=arm CROSS_COMPILE=arm-eabi- $*
}

# a quick command to clean u-boot building files
function uclean()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	make -C $T/u-boot O=$OUT/obj/u-boot mrproper
	make -C $T/u-boot O=$OUT/obj/pdl mrproper
}

# a quick command to clean u-boot building files
function umk()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	make -C $T/u-boot O=$OUT/obj/u-boot $*
}

# a quick command to config u-boot
function uconfig()
{
	T=$(gettop)
	if [ ! "$T" ]; then
		echo "Couldn't locate the top of the tree.  Try setting TOP."
		return
	fi

	make -C $T/u-boot O=$OUT/obj/u-boot \
	$(get_build_var UBOOT_DEFCONFIG)_config
}

function bsphelp() {
cat <<EOF
This is the functions build for kernel and u-boot
- kheader: 	Install kernel header files for userspace building
- kmconfig:     Launch kernel menuconfig
- kuconfig:     Launch kernel menuconfig and update .config to kernel
- kdconfig:     Config kernel to the corresponding defconfig
- kclean:   	Clean kernel building files
- kmk:  	Run kernel make
- uclean: 	Clean u-boot building files
- uconfig:   	Config u-boot
- umk:		Run u-boot make
Look at the source to view more functions. The complete list is:
EOF
    T=$(gettop)
    A=""
    for i in kmk kuconfig kclean uclean umk uconfig kmconfig; do
      A="$A $i"
    done
    echo $A
}

function lunchhelp() {
cat <<EOF
This is a simple description for rdadroid projects
- etau:         Phone, HVGA/WVGA/FWVGA,(mainly for 8810P and 8810N)
  +RDA 8860EP_A6B_V2
   chip: 8860EP
   crystal oscillator
   wifi: 5991g
   camera: GC0312+GC0312
   screen: ST7796   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +RDA 8860EP_A6B_V3
   chip: 8860EP
   vc_tcxo
   wifi: 5991e
   camera: GC0312+GC0312
   screen: ST7796   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulmode 3g
  +RDA 8860EP_A6B_V2_3G
   chip: 8860EP
   crystal oscillator
   wifi: 5991g
   camera: GC0312+GC0312
   screen: ST7796   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g(support 3g)
  +RDA 8860EP_A6B_V3_3G
   chip: 8860EP
   vc_tcxo
   wifi: 5991e
   camera: GC0312+GC0312
   screen: ST7796   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulmode 3g(support 3g)
  +RDA 8850E_DEV3_3G
   chip: 8850E
   vc_tcxo
   wifi: 5991g
   camera: GC2155+GC0329
   screen: ILI9488/HX8357   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 3g
  +RDA 8850E_DEV3_GSM
   chip: 8850E
   vc_tcxo
   wifi: 5991g
   camera: GC2155+GC0329
   screen: ILI9488/HX8357   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +RDA 8850E_M_DDR_GSM
   chip: 8850E
   vc_tcxo
   wifi: 5991e
   camera: GC2155+GC0329
   screen: ILI9488/HX8357   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +RDA 8850E_M_DDR
   chip: 8850E
   vc_tcxo
   wifi: 5991e
   camera: GC2155+GC0329
   screen: ILI9488/HX8357   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 3g
  +RDA 8850EM
   chip: 8850E
   vc_tcxo
   wifi: 5991e
   camera: GC0329+GC0329
   screen: HIMAX_8379C   4.0inchs   480*800  MIPI
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +RDA 8850E
   chip: 8850E
   crystal oscillator
   wifi: 5991e
   camera: GC2155+GC0329
   screen: ILI9488/HX8357   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +RDA 8860EP_A6B
   chip: 8860EP
   crystal oscillator
   wifi: 5991g
   camera: GC2155+GC0329
   screen: RM68180/ILI9488   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +NollecA8V2V8810P
   chip: 8810P
   vc_tcxo
   wifi: 5990p
   camera: GC0329+GC0329
   screen: R61581B/ILI9488/RM68140/NT35310   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +NollecA9V2V8810P
   chip: 8810P
   vc_tcxo
   wifi: 5991e
   camera: GC0312+GC0329
   screen: R61581B/ILI9488/RM68140/NT35310   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +NollecA11S20V8810P
   chip: 8810P
   vc_tcxo
   wifi: 5991
   camera: GC0328+GC0309
   screen: ILI9806H   3.5inchs   480*800  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 202144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +RDA8860EP_A6B_MIPI_V0
   chip: 8850E42p
   vc_tcxo
   wifi: 5991g
   camera: GC0328C(back)+GC0329(front)
   screen: HX8379C_BOE397/OTM8019A_BOE397/RM68172(AUTO)   3.5inchs   480*80  MIPI
   nand: 4Gbit   pagesize 4096byte       blocksize 262144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +NollecA9V2V8811P
   chip: 8811P
   crystal oscillator
   wifi: 5991g
   camera: GC0312/GC0329/BF3703(back)
   screen: R61581B/ILI9488/RM68140/NT35310(HVGA)   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 12288byte       blocksize 3145728byte
   ddr:  4Gbit
   modem: doulemode 2g
  +RDA8850E_DEV3_3G_NOANDROID
   chip: 8850E42P
   vc_tcxo
   wifi: 5991g
   camera: GC2155(back)
   screen: ILI9488/HX8357(HVGA)  3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 262144byte
   ddr:  2Gbit
   modem: doulemode 3g
  +Lensun_R635D_8810P
   chip: 8810P
   crystal oscillator
   wifi: 5991g
   camera: GC0328(back)+GC0329(front)
   screen: ILI9488/RM68140/HX8357(HVGA)   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 262144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +RDA8850E_DEV3_3G_NOANDROID_U02
   chip: 8850E
   vc_tcxo
   wifi: 5991g
   camera: GC2155(back)
   screen: ILI9488/HX8357(HVGA)   3.5inchs   320*480  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 262144byte
   ddr:  2Gbit
   modem: doulemode 3g
  +TinnoV2000AN
   chip: 8850E42P
   vc_tcxo
   wifi: 5991g
   camera: SP2508_CSI(back)
   screen: HX8379C(HVGA)   3.5inchs   320*480  MIPI
   nand: 4Gbit   pagesize 4096byte       blocksize 262144byte
   ddr:  2Gbit
   modem: doulemode 2g
  +NollecA11S15V8810P
   chip: 8810P
   crystal oscillator
   wifi: 5991g
   camera: GC0328(back)+GC0309(front)
   screen: RM68180/ILI9806H(WVGA)   3.5inchs   480*800  MCU
   nand: 4Gbit   pagesize 4096byte       blocksize 262144byte
   ddr:  2Gbit
   modem: doulemode 2g
- haiyan:       Pad, WVGA L, UBIFS 4K
  + BP786:        KPF BP786 target (32+4)
  + CR711:        PH CR711 target (32+4)
  + R70E:         PW R70E target (32+4)
  + X3V3:         WF X3V3 target (32+4)
  + X3V4P3:       WF X3V4P3 target (32+4)
- jebi:         Pad, WVGA L, EMMC/EMCP with ext4
  +RDA 8850E
   chip: 8850E
   crystal oscillator
   wifi: 5991e
   camera: GC2155+GC0329
   screen: ILI9488/HX8357   3.5inchs   320*480  MCU
   emmc: 2GB--inf
   ddr:  2Gbit
   modem: doulemode 2g
  +R6B_RDA8850EV4_MIPI_GSM
   chip: 8850E42P
   vc_tcxo
   wifi: 5991g
   camera: GC2145_CSI(back)+GC0310_CSI(front)
   screen: HX8379C_BOE397/OTM8019A_BOE397/RM68172(WVGA)   3.5inchs   480*80  MIPI
   nand: 4Gbit   pagesize 4096byte       blocksize 262144byte
   ddr:  2Gbit
   modem: doulemode 2g
- krosa:        Phone, WVGA, EMMC/EMCP with ext4
  + R7629P2E:     R7629 P2E EMCP target (32+4)
EOF
}

function findtarget()
{
    unset TARGET_MENU_CHOICES
    local oldpwd=$OLDPWD
    local pwd=$PWD
    local targets=$ANDROID_BUILD_TOP/device/rda/$TARGET_PRODUCT 

    if [ -d $targets ]
    then
	    cd $targets > /dev/null
    else
	    return 1
    fi

    if (ls */target.def > /dev/null 2>&1)
    then
        if [ "x`gettargeterrlinks`" != "x" ]
        then
            cd $pwd > /dev/null; OLDPWD=$oldpwd
            return 2
        fi
    else
        cd $pwd > /dev/null; OLDPWD=$oldpwd
        return 1
    fi

    for dir in */target.def
    do
        local string=`dirname $dir`
        TARGET_MENU_CHOICES=(${TARGET_MENU_CHOICES[@]} $string)
    done

    cd $pwd > /dev/null; OLDPWD=$oldpwd

    if [ ${#TARGET_MENU_CHOICES[*]} -gt 0 ]
    then
        return 0
    fi

    return 1
}

function printtarget()
{
    local i=1
    local choice

    for choice in ${TARGET_MENU_CHOICES[@]}
    do
        if [ "x$TARGET_HARDWARE_CFG" = "x$choice" ]
        then
            echo " *   $i. $choice"
        else
            echo "     $i. $choice"
        fi
        i=$(($i+1))
    done
}

#RDA_TARGET_LINK_LIST="include target.def customer.mk oem_driver.sh camera.cfg"

function gettargeterrlinks()
{
    local errlinks

   echo $errlinks
}


function choosetarget()
{
    local oldpwd=$OLDPWD
    local pwd=$PWD
    local index=
    local target=$1

    if [ -z "$ANDROID_BUILD_TOP" ] || [ -z "$TARGET_PRODUCT" ]
    then
        return 1
    fi

    if [ -z "$target" ]
    then
        return 2
    fi

    if (echo -n $target | grep -q -e "^[^\-][^\-]*$")
    then
        target=$1
        local i=0
        for m in ${TARGET_MENU_CHOICES[@]}
        do
            if [ "$m" = "$target" ]
            then
                index=$i
                break
            fi
            i=$(($i+1))
        done
    fi

    if [ -z "$index" ]
    then
        if (echo -n $1 | grep -q -e "^[1-9][0-9]*$")
        then
            if [ $1 -le ${#TARGET_MENU_CHOICES[*]} ]
            then
                index=$(($1-1))
            fi
        fi
    fi

    if [ -z "$index" ]
    then
        return 3
    fi

    export TARGET_HARDWARE_CFG=${TARGET_MENU_CHOICES[$index]}
    export RDA_TARGET_DEVICE_DIR=device/rda/${TARGET_PRODUCT}/${TARGET_HARDWARE_CFG}
    return 0
}

function targethelp() {
cat <<EOF
function list:
choosetarget printtarget

Simple description for rdadroid projects' target selection
- etau
  + P6U2G
  + NollecA8
  + NollecA8_v2
- fitow
  + R7629U
  + R7629V3U
  + R7629V4U
  + R7629V8810D
  + WLDS301P3
- goni
  + CR985
  + R7629U2G
  + R7629V3U2G
  + R7629P2AV3
  + R7629V4U2G
  + R7629V8810M
  + HX7800
- haiyan
  + BP786
  + CR711
  + R70E
  + X3V3
  + X3V4P3
- jebi
  + X3V3
  + X3V4P3
  + KA1102
- krosa
  + R7629P2E
- lupit
  + R70E
EOF
}

function cdtop()
{
    T=$(gettop)
    if [ "$T" ]; then
        cd "$T/$1"
    else
        echo "Couldn't locate the top of the tree.  Try setting TOP."
        return 1
    fi
}

function cdmodem()
{
	modem_type=`get_build_var MODEM_TYPE`
	if [ "$modem_type" ]; then
		cd "$ANDROID_BUILD_TOP/modem/modem-$modem_type"
	else
		echo "failed to find modem directory"
		return 1;
	fi
}

function msync()
{
	cdmodem
	./modem_sync.sh
	croot
}
alias cdk='cdtop kernel'
alias cdu='cdtop u-boot'
alias cdm='cdtop modem'
alias cdcust='cdtop device/rda/driver'
alias cdtgt='cdtop device/rda/$TARGET_PRODUCT/$TARGET_HARDWARE_CFG'
alias cdobj='cd $ANDROID_PRODUCT_OUT/obj'
alias cdout='cd $ANDROID_PRODUCT_OUT'

