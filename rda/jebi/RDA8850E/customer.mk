
# touch screen selection
RDA_CUSTOMER_DRV_TS := TS_GSL168X
#  GSL168X_MODEL := GSL168X_MODEL_DUOCAI_1680E
#  GSL168X_MODEL := GSL168X_MODEL_35_HVGA_1680
#  GSL168X_MODEL := GSL168X_MODEL_35_HVGA_1688E
#  GSL168X_MODEL := GSL168X_MODEL_40_WVGA
GSL168X_MODEL	:=  GSL168X_MODEL_70_WVGAL_D70E
# RDA_CUSTOMER_DRV_TS := TS_GSL1688E
# RDA_CUSTOMER_DRV_TS := TS_MSG2133
# RDA_CUSTOMER_DRV_TS := TS_FT6X06
# RDA_CUSTOMER_DRV_TS := TS_IT7252
# RDA_CUSTOMER_DRV_TS := TS_GTP868
# RDA_CUSTOMER_DRV_TS := TS_ICN831X

# gsensor selection
RDA_CUSTOMER_DRV_GS := GS_MMA865X
# RDA_CUSTOMER_DRV_GS := GS_MMA7660
# RDA_CUSTOMER_DRV_GS := GS_STK8312
# RDA_CUSTOMER_DRV_GS := GS_MMA845X

# light sensor selection
RDA_CUSTOMER_DRV_LS := LS_NONE

#camera sensor selection
# supported list in device/rda/driver/camera/inc/
RDA_CUSTOMER_DRV_CSB := GC2035
RDA_CUSTOMER_DRV_CSF := GC0328

#DRC selection
BOARD_SUPPORT_DRC := true

# new cat
BOARD_SUPPORT_AUDIO_DYNAMIC_CALIB := true

BOARD_NAND_PAGE_SIZE := 4096

export RDA_CUSTOMER_DRV_TS
ifeq ($(RDA_CUSTOMER_DRV_TS), TS_GSL168X)
export GSL168X_MODEL
endif
export RDA_CUSTOMER_DRV_GS
export RDA_CUSTOMER_DRV_LS
export RDA_CUSTOMER_DRV_CSB
export RDA_CUSTOMER_DRV_CSF

