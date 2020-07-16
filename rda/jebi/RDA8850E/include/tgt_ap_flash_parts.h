#ifndef __TGT_AP_FLASH_PARTS_H__
#define __TGT_AP_FLASH_PARTS_H__

#define MTDPARTS_DEF			\
		"384K@128K(bootloader),"	\
		"512K(factorydata),"	\
		"1M(misc),"	\
		"7M(modem),"		\
		"8M(boot),"		\
		"8M(recovery),"		\
		MTDPARTS_ANDROID_DEF

#define MTDPARTS_ANDROID_DEF		\
		"8M(kpanic),"		\
		"330M(system),"		\
		"336M(vendor),"	\
		"1000M(userdata),"	\
		"-(fat)"
/*
kernel  need handle mtd from 0, so define a dummy partions whose
size is bootloader+factorydata+modem+boot+recovery
*/
/*
#define MTDPARTS_KERNEL_DEF		\
		"25M@0(dummy),"		\
		MTDPARTS_ANDROID_DEF
*/
#define MTDPARTS_KERNEL_DEF	MTDPARTS_DEF
#endif