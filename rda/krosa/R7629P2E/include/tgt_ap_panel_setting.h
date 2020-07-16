#ifndef __TGT_AP_PANEL_SETTING_H
#define __TGT_AP_PANEL_SETTING_H

#include "rda_panel_comm.h"

/* ***************************************************************
   * The above list all the panel rda support, the following line to
   * select panel 
   ***************************************************************
*/ 

#define PANEL_NAME	ILI9806H_MCU_PANEL_NAME
#define PANEL_XSIZE	WVGA_LCDD_DISP_X
#define PANEL_YSIZE	WVGA_LCDD_DISP_Y

/*
 * display direction setting.
 */
#define DISPLAY_DIRECTION_180_MODE

#define TARGET_AP_PANEL_ILI9806H_MCU	1
#endif /* __TGT_AP_PANEL_SETTING_H */
