#ifndef __TGT_AP_PANEL_SETTING_H
#define __TGT_AP_PANEL_SETTING_H

#include "rda_panel_comm.h"

/* ***************************************************************
   * The above list all the panel rda support, the following line to
   * select panel 
   ***************************************************************
*/ 
#define PANEL_NAME	AUTO_DET_LCD_PANEL_NAME
#define PANEL_XSIZE	HVGA_LCDD_DISP_X
#define PANEL_YSIZE	HVGA_LCDD_DISP_Y

/* for multiple panel support, user can set AUTO_DETECR_SUPPORTED_PANEL_NUM to -1 and let kernel lookup all panel */
#undef AUTO_DETECT_SUPPORTED_PANEL_NUM
#undef AUTO_DETECT_SUPPORTED_PANEL_LIST

#define AUTO_DETECT_SUPPORTED_PANEL_NUM  4

#define AUTO_DETECT_SUPPORTED_PANEL_LIST \
		R61581B_MCU_PANEL_NAME  \
		ILI9488_MCU_PANEL_NAME	\
		RM68140_MCU_PANEL_NAME 	\
		NT35310_MCU_PANEL_NAME

#define TARGET_AP_PANEL_R61581B_MCU	1
#define TARGET_AP_PANEL_ILI9488_MCU	1
#define TARGET_AP_PANEL_RM68140_MCU	1
#define TARGET_AP_PANEL_NT35310_MCU	1
#endif /* __TGT_AP_PANEL_SETTING_H */
