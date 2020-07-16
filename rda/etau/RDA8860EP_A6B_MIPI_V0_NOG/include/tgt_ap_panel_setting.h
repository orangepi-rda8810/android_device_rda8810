#ifndef __TGT_AP_PANEL_SETTING_H
#define __TGT_AP_PANEL_SETTING_H

#include "rda_panel_comm.h"

/* ***************************************************************
   * The above list all the panel rda support, the following line to
   * select panel
   ***************************************************************
*/

#define PANEL_NAME	AUTO_DET_LCD_PANEL_NAME
#define PANEL_XSIZE	AUTO_LCDD_WVGA_DISP_X
#define PANEL_YSIZE	AUTO_LCDD_WVGA_DISP_Y
//#define PANEL_XSIZE	WVGA_LCDD_DISP_X
//#define PANEL_YSIZE	WVGA_LCDD_DISP_Y
//#define PANEL_XSIZE	FWVGA_LCDD_DISP_X
//#define PANEL_YSIZE	FWVGA_LCDD_DISP_Y
//#define PANEL_XSIZE	WVGA_LCDD_DISP_X
//#define PANEL_YSIZE	WVGA_LCDD_DISP_Y
//#define PANEL_XSIZE	WVGA_LCDD_DISP_X
//#define PANEL_YSIZE	WVGA_LCDD_DISP_Y
/* for multiple panel support, user can set AUTO_DETECR_SUPPORTED_PANEL_NUM to -1 and let kernel lookup all panel */
#undef AUTO_DETECT_SUPPORTED_PANEL_NUM
#undef AUTO_DETECT_SUPPORTED_PANEL_LIST

#define AUTO_DETECT_SUPPORTED_PANEL_NUM  3

#define AUTO_DETECT_SUPPORTED_PANEL_LIST \
		ASPACE(HX8379C_BOE397_MIPI_PANEL_NAME)	\
		ASPACE(OTM8019A_BOE397_MIPI_PANEL_NAME)	\
		ASPACE(RM68172_MIPI_PANEL_NAME)

/*For rda panel driver init*/
//#define TGT_AP_PANEL_OTM8019A_MIPI		1
//#define TGT_AP_PANEL_HIMAX_8379C_MIPI_PANEL	1
#define TGT_AP_PANEL_RM68172_MIPI		1
#define TGT_AP_PANEL_HX379C_BOE397_MIPI 	1
#define TGT_AP_PANEL_OTM8019A_BOE397_MIPI	1
#endif /* __TGT_AP_PANEL_SETTING_H */
