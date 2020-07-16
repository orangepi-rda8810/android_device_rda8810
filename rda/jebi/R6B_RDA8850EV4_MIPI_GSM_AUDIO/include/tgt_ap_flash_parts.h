#ifndef __TGT_AP_FLASH_PARTS_H__
#define __TGT_AP_FLASH_PARTS_H__

#define MTDPARTS_DEF			\
		"1920K@128K(bootloader),"	\
		"1M(factorydata),"	\
		"1M(misc),"	\
		"12M(modem),"		\
		"8M(boot),"		\
		"24M(recovery),"		\
		MTDPARTS_ANDROID_DEF

#define MTDPARTS_ANDROID_DEF		\
		"8M(kpanic),"		\
		"330M(system),"		\
		"402M(vendor),"	\
		"-(userdata)"


/*
kernel  need handle mtd from 0, so define a dummy partions whose
size is bootloader+factorydata+modem+boot+recovery
*/
/*
#define MTDPARTS_KERNEL_DEF		\
		"48M@0(dummy),"		\
		MTDPARTS_ANDROID_DEF
*/
#define MTDPARTS_KERNEL_DEF	MTDPARTS_DEF
#endif

