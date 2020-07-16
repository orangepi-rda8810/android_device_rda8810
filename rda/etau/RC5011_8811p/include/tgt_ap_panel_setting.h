#ifndef __TGT_AP_PANEL_SETTING_H
#define __TGT_AP_PANEL_SETTING_H

#include "rda_panel_comm.h"

/* ***************************************************************
   * The above list all the panel rda support, the following line to
   * select panel 
   ***************************************************************
*/ 

//#define TGT_FB_IS_32BITS

//#define PANEL_NAME	JB070SZ03A_PANEL_NAME
//#define PANEL_XSIZE	WVGA_LCDD_DISP_Y
//#define PANEL_YSIZE	WVGA_LCDD_DISP_X

#define PANEL_NAME	OTM8019A_PANEL_NAME
#define PANEL_XSIZE	OTM8019A_LCDD_DISP_X
#define PANEL_YSIZE	OTM8019A_LCDD_DISP_Y


//#define TGT_LCD_DATA_IS_24BITS
#define DISPLAY_DIRECTION_180_MODE

#define TARGET_AP_PANEL_OTM8019A_RGB	1
#endif /* __TGT_AP_PANEL_SETTING_H */
