#ifndef __TGT_AP_FLASH_PARTS_H__
#define __TGT_AP_FLASH_PARTS_H__

#define MTDPARTS_DEF			\
		"2M@0(bootloader),"	\
		"2M(factorydata),"	\
		"4M(modem),"		\
		"8M(boot),"		\
		"10M(recovery),"		\
		MTDPARTS_ANDROID_DEF

#define MTDPARTS_ANDROID_DEF		\
		"120M(system),"		\
		"90M(vendor),"     \
		"-(userdata)"

/*
kernel  need handle mtd from 0, so define a dummy partions whose
size is bootloader+factorydata+modem+boot+recovery
*/
#define MTDPARTS_KERNEL_DEF		\
		"26M@0(dummy),"		\
		MTDPARTS_ANDROID_DEF
#endif
