#ifndef __TGT_AP_PANEL_SETTING_H
#define __TGT_AP_PANEL_SETTING_H

#include "rda_panel_comm.h"

/* ***************************************************************
   * The above list all the panel rda support, the following line to
   * select panel
   ***************************************************************
*/

#define PANEL_NAME	AUTO_DET_LCD_PANEL_NAME
//#define PANEL_XSIZE	HVGA_LCDD_DISP_X
//#define PANEL_YSIZE	HVGA_LCDD_DISP_Y
#define PANEL_XSIZE	WVGA_LCDD_DISP_X
#define PANEL_YSIZE	WVGA_LCDD_DISP_Y
//#define PANEL_XSIZE	FWVGA_LCDD_DISP_X
//#define PANEL_YSIZE	FWVGA_LCDD_DISP_Y
//#define PANEL_XSIZE	WVGA_LCDD_DISP_X
//#define PANEL_YSIZE	WVGA_LCDD_DISP_Y
//#define PANEL_XSIZE	WVGA_LCDD_DISP_X
//#define PANEL_YSIZE	WVGA_LCDD_DISP_Y
/* for multiple panel support, user can set AUTO_DETECR_SUPPORTED_PANEL_NUM to -1 and let kernel lookup all panel */
#undef AUTO_DETECT_SUPPORTED_PANEL_NUM
#undef AUTO_DETECT_SUPPORTED_PANEL_LIST

#define AUTO_DETECT_SUPPORTED_PANEL_NUM  1

#define AUTO_DETECT_SUPPORTED_PANEL_LIST \
		ASPACE(JD9161BA_MIPI_PANEL_NAME)

//ASPACE(ILI9806E_MIPI_PANEL_NAME)

/*For rda panel driver init*/
//#define TGT_AP_PANEL_ILI9806E_MIPI	1
#define TGT_AP_PANEL_JD9161BA_MIPI      1
#endif /* __TGT_AP_PANEL_SETTING_H */
