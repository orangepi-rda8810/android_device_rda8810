#ifndef __TGT_AP_FLASH_PARTS_H__
#define __TGT_AP_FLASH_PARTS_H__

#define MTDPARTS_DEF			\
		"4M@0(bootloader),"	\
		"4M(factorydata),"	\
		"4M(misc),"	        \
		"12M(modem),"		\
		"8M(boot),"		\
		"24M(recovery),"		\
		MTDPARTS_ANDROID_DEF

#define MTDPARTS_ANDROID_DEF  \
		"150M(system),"		\
		"100M(vendor),"	\
		"-(userdata)"


/*
kernel  need handle mtd from 0, so define a dummy partions whose
size is bootloader+factorydata+modem+boot+recovery
*/
/*
#define MTDPARTS_KERNEL_DEF		\
		"56M@0(dummy),"		\
		MTDPARTS_ANDROID_DEF
*/
#define MTDPARTS_KERNEL_DEF	MTDPARTS_DEF
#endif