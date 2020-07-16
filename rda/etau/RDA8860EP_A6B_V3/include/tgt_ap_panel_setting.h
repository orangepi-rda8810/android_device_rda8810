#ifndef __TGT_AP_PANEL_SETTING_H
#define __TGT_AP_PANEL_SETTING_H

#include "rda_panel_comm.h"

/* ***************************************************************
   * The above list all the panel rda support, the following line to
   * select panel
   ***************************************************************
*/
#define __PANEL_IDENTIFY_BY_VOLTAGE__

#ifdef  __PANEL_IDENTIFY_BY_VOLTAGE__
#define LCD_ID_GPADC_CHANNEL          2
#define LCD_ID_VOLTAGE_0_6V     		600
#define LCD_ID_VOLTAGE_1_8V     		1800
#define LCD_ID_VOLTAGE_NC_0V   	0
#define LCD_ID_VOLTAGE_DEBONCE_RANGE	200
#define LCD_ID_GPADC_STABLE_WAIT	  100
#define LCD_ID_GPADC_READ_SPACE	  50
#endif

#define PANEL_NAME	AUTO_DET_LCD_PANEL_NAME
#define PANEL_XSIZE	HVGA_LCDD_DISP_X
#define PANEL_YSIZE	HVGA_LCDD_DISP_Y

/* for multiple panel support, user can set AUTO_DETECR_SUPPORTED_PANEL_NUM to -1 and let kernel lookup all panel */
#undef AUTO_DETECT_SUPPORTED_PANEL_NUM
#undef AUTO_DETECT_SUPPORTED_PANEL_LIST

#define AUTO_DETECT_SUPPORTED_PANEL_NUM  2

#define AUTO_DETECT_SUPPORTED_PANEL_LIST \
			ASPACE(ST7796_MCU_PANEL_NAME)     \
			ASPACE(ILI9488_MCU_PANEL_NAME)
/*For lcd panel driver init*/
//#define TGT_AP_PANEL_HX8357_MCU 1
#define TGT_AP_PANEL_ILI9488_MCU_PANEL 1
#define TGT_AP_PANEL_ST7796_MCU 1

#endif /* __TGT_AP_PANEL_SETTING_H */
