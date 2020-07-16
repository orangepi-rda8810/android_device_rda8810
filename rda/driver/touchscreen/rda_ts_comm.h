#ifndef __RDA_TS_COMM_H
#define __RDA_TS_COMM_H

/* definition of msg2133 touch screen */
#define MSG2133_RES_X   2048
#define MSG2133_RES_Y   2048

#if 0
#define GSL168X_PROC_DEBUG
#endif /* #if 0 */

/* definition of gsl1680 and gsl1680e touch screen */
#ifdef GSL168X_MODEL_35_HVGA_1680
#define GSL168X_RES_X   320
#define GSL168X_RES_Y   480
#elif defined(GSL168X_MODEL_35_HVGA_1688E)
#define GSL168X_RES_X   320
#define GSL168X_RES_Y   480
#elif defined(GSL168X_MODEL_35_HVGA_1688E_KRTR118)
#define GSL168X_RES_X   320
#define GSL168X_RES_Y   480
#elif defined(GSL168X_MODEL_40_WVGA_ORANGEPI)
#define GSL168X_RES_X   480
#define GSL168X_RES_Y   800
#elif defined(GSL168X_MODEL_40_WVGA)
#define GSL168X_RES_X   480
#define GSL168X_RES_Y   800
#elif defined(GSL168X_MODEL_70_WVGAL)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_Y
#elif defined(GSL168X_MODEL_DUOCAI_1680E)
#define GSL168X_RES_X   480
#define GSL168X_RES_Y   800
#elif defined(GSL168X_MODEL_70_WVGAL_B)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_Y
#elif defined(GSL168X_MODEL_70_WVGAL_CMW)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_Y
#elif defined(GSL168X_MODEL_70_WVGAL_BP786)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_Y
#elif defined(GSL168X_MODEL_70_WVGAL_GLS2439)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_X
#elif defined(GSL168X_MODEL_70_WVGAL_D70E)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_X
#elif defined(GSL168X_MODEL_70_WVGAL_D706E)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_X
#elif defined(GSL168X_MODEL_WVGA_X3)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_Y
#elif defined(GSL168X_MODEL_WVGA_X3V4)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_Y
#elif defined(GSL168X_MODEL_70_WVGAL_BP605)
#define GSL168X_RES_X   800
#define GSL168X_RES_Y   480
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_X
#elif defined(GSL168X_MODEL_70_WSVGAL_BP605)
#define GSL168X_RES_X   1024
#define GSL168X_RES_Y   600
#define GSL168X_INVERT_XY
#define GSL168X_INVERT_X
#else
#undef GSL168X_PROC_DEBUG
#endif

/* definition of gsl1680 and gsl1680e touch screen */
#define IT7252_RES_X    320
#define IT7252_RES_Y    480

/* definition of gtp868 touch screen */
#define GTP868_RES_X    480
#define GTP868_RES_Y    800

/* definition of ICN8139M touch screen */
#define ICN831X_RES_X   768
#define ICN831X_RES_Y   1024

#endif /* __TGT_TS_COMM_H */
