#ifndef _TGT_AP_CLOCK_CFG_H_
#define _TGT_AP_CLOCK_CFG_H_

/*
 * PLL Freqs
 */
#define OVPHONE_MEM_FREQ_156 0
#define OVPHONE_MEM_FREQ_260 1
#define OVPHONE_MEM_FREQ_290 0
#define OVPHONE_MEM_FREQ_312 0
#define OVPHONE_MEM_FREQ_333 0
#define OVPHONE_MEM_FREQ_351 0
#define OVPHONE_MEM_FREQ_355 0

#define _TGT_AP_PLL_CPU_FREQ    (988)
#define _TGT_AP_PLL_BUS_FREQ    (800)

#if(OVPHONE_MEM_FREQ_156)
#define _TGT_AP_PLL_MEM_FREQ    (156)
#elif (OVPHONE_MEM_FREQ_260)
#define _TGT_AP_PLL_MEM_FREQ    (260)
#elif (OVPHONE_MEM_FREQ_290)
#define _TGT_AP_PLL_MEM_FREQ    (290)
#elif (OVPHONE_MEM_FREQ_312)
#define _TGT_AP_PLL_MEM_FREQ    (312)
#elif (OVPHONE_MEM_FREQ_333)
#define _TGT_AP_PLL_MEM_FREQ    (333)
#elif (OVPHONE_MEM_FREQ_351)
#define _TGT_AP_PLL_MEM_FREQ    (351)
#elif (OVPHONE_MEM_FREQ_355)
#define _TGT_AP_PLL_MEM_FREQ    (355)
#endif

#define _TGT_AP_PLL_USB_FREQ    (480)

/*
 * DDR settings
 */
/* DDR clock rate (data rate double this number) */
#define _TGT_AP_DDR_TYPE        3
#if (_TGT_AP_PLL_MEM_FREQ == 400) \
	|| (_TGT_AP_PLL_MEM_FREQ == 355) \
	|| (_TGT_AP_PLL_MEM_FREQ == 351) \
	|| (_TGT_AP_PLL_MEM_FREQ == 333) \
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
#define _TGT_AP_DDR_ROW_BITS    (4)
#define _TGT_AP_DDR_COL_BITS    (2)
/* DDR timing */
#if(OVPHONE_MEM_FREQ_156)
#include "ddr_timing/micron_16x2_156m_weifu.h"
#elif (OVPHONE_MEM_FREQ_260)
#if (_TGT_AP_DDR_WIDTH == 32)
#include "ddr_timing/micron_16x2_260m_weifu_2.h"
#else
#include "ddr_timing/micron_16x1_260m_weifu.h"
#endif
#elif (OVPHONE_MEM_FREQ_290)
#if (_TGT_AP_DDR_WIDTH == 32)
#include "ddr_timing/micron_16x2_290m_weifu.h"
#else
#include "ddr_timing/micron_16x1_290m_weifu.h"
#endif
#elif (OVPHONE_MEM_FREQ_312)
#if (_TGT_AP_DDR_WIDTH == 32)
#include "ddr_timing/micron_16x2_312m_weifu.h"
#else
#include "ddr_timing/micron_16x1_312m_weifu.h"
#endif
#elif (OVPHONE_MEM_FREQ_333)
#if (_TGT_AP_DDR_WIDTH == 32)
#include "ddr_timing/micron_16x2_333m_weifu.h"
#else
#include "ddr_timing/micron_16x1_333m_weifu.h"
#endif
#elif (OVPHONE_MEM_FREQ_351)
#if (_TGT_AP_DDR_WIDTH == 32)
#include "ddr_timing/micron_16x2_351m_weifu.h"
#else
#include "ddr_timing/micron_16x1_351m_weifu.h"
#endif
#elif (OVPHONE_MEM_FREQ_355)
#if (_TGT_AP_DDR_WIDTH == 32)
#include "ddr_timing/micron_16x2_355m_weifu.h"
#else
#include "ddr_timing/micron_16x1_355m_weifu.h"
#endif
#endif

/*
 * CLK settings
 */
#define _TGT_AP_CLK_CPU         (0x001F)
#define _TGT_AP_CLK_AXI         (0x001E)
#define _TGT_AP_CLK_GCG         (0x001E)
#define _TGT_AP_CLK_AHB1        (0x001A)
#define _TGT_AP_CLK_APB1        (0x001A)
#define _TGT_AP_CLK_APB2        (0x001A)
#define _TGT_AP_CLK_MEM         (0x0000)
#define _TGT_AP_CLK_GPU         (0x001A)
#define _TGT_AP_CLK_VPU         (0x0016)
#define _TGT_AP_CLK_VOC         (0x001A)
#define _TGT_AP_CLK_SFLSH       (0x001D)

#endif //_TGT_AP_CLOCK_CFG_H_

