#ifndef _TGT_AP_CLOCK_CFG_H_
#define _TGT_AP_CLOCK_CFG_H_

/*
 * PLL Freqs
 */
#define _TGT_AP_PLL_CPU_FREQ    (936)
#define _TGT_AP_PLL_BUS_FREQ    (800)
#define _TGT_AP_PLL_MEM_FREQ    (312)
#define _TGT_AP_PLL_USB_FREQ    (480)

/*
 * DDR settings
 */
/* DDR clock rate (data rate double this number) */
#define _TGT_AP_DDR_TYPE        2
#if (_TGT_AP_PLL_MEM_FREQ == 400) \
	|| (_TGT_AP_PLL_MEM_FREQ == 533) \
	|| (_TGT_AP_PLL_MEM_FREQ == 519) \
	|| (_TGT_AP_PLL_MEM_FREQ == 455) \
	|| (_TGT_AP_PLL_MEM_FREQ == 416) \
	|| (_TGT_AP_PLL_MEM_FREQ == 355) \
	|| (_TGT_AP_PLL_MEM_FREQ == 351) \
	|| (_TGT_AP_PLL_MEM_FREQ == 333) \
	|| (_TGT_AP_PLL_MEM_FREQ == 325) \
	|| (_TGT_AP_PLL_MEM_FREQ == 312) \
	|| (_TGT_AP_PLL_MEM_FREQ == 290) \
	|| (_TGT_AP_PLL_MEM_FREQ == 260) \
	|| (_TGT_AP_PLL_MEM_FREQ == 200) \
	|| (_TGT_AP_PLL_MEM_FREQ == 156) \
	|| (_TGT_AP_PLL_MEM_FREQ == 100)
#define _TGT_AP_DDR_CLOCK       _TGT_AP_PLL_MEM_FREQ
#else
#error "Invalid DDR_CLOCK"
#endif
#define _TGT_AP_DDR_LOWPWR
#define _TGT_AP_DDR_ODT         (2)
#define _TGT_AP_DDR_RON         (0)
#define _TGT_AP_DDR_CHIP_BITS   (0)
/* DDR bus width, valid value are 8, 16 or 32 */
#define _TGT_AP_DDR_WIDTH       (32)
#if (_TGT_AP_DDR_WIDTH == 8)
#define _TGT_AP_DDR_MEM_BITS    (0)
#elif (_TGT_AP_DDR_WIDTH == 16)
#define _TGT_AP_DDR_MEM_BITS    (1)
#elif (_TGT_AP_DDR_WIDTH == 32)
#define _TGT_AP_DDR_MEM_BITS    (2)
#else
#error "Invalid DDR WIDTH"
#endif
#define _TGT_AP_DDR_BANK_BITS   (3)
#define _TGT_AP_DDR_ROW_BITS    (3)
#define _TGT_AP_DDR_COL_BITS    (1)
/* DDR timing */
/*#include "ddr_timing/8850e_16x2_325m.h"*/
/*#include "ddr_timing/8850e_16x2_333m_eco2.h"*/
#include "ddr_timing/8850e_16x2_312m_eco2.h"

/*
 * CLK settings
 */
#define _TGT_AP_CLK_CPU         (0x001F)
#define _TGT_AP_CLK_AXI         (0x001E)
#define _TGT_AP_CLK_GCG         (0x001E)
#define _TGT_AP_CLK_AHB1        (0x001A)
#define _TGT_AP_CLK_APB1        (0x0006)
#define _TGT_AP_CLK_APB2        (0x001A)
#define _TGT_AP_CLK_MEM         (0x0000)
#define _TGT_AP_CLK_GPU         (0x001A)
#define _TGT_AP_CLK_VPU         (0x0016)
#define _TGT_AP_CLK_VOC         (0x001A)
#define _TGT_AP_CLK_SFLSH       (0x001D)

#endif //_TGT_AP_CLOCK_CFG_H_

