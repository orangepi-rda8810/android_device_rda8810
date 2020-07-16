
#4k
BOARD_NAND_PAGE_SIZE := 4096
BOARD_NAND_SPARE_SIZE := 218
BOARD_FLASH_BLOCK_SIZE := 262144
BOARD_NAND_ECCMSGLEN := 1024
BOARD_NAND_ECCBITS := 24
BOARD_NAND_OOB_SIZE := 32

# touch screen selection
# RDA_CUSTOMER_DRV_TS := TS_GSL168X
#  GSL168X_MODEL := GSL168X_MODEL_35_HVGA_1680
#  GSL168X_MODEL := GSL168X_MODEL_35_HVGA_1688E
#  GSL168X_MODEL := GSL168X_MODEL_40_WVGA
  RDA_CUSTOMER_DRV_TS := TS_MSG2133

# RDA_CUSTOMER_DRV_TS := TS_FT6X06
#  TS_FT6X06_MODEL := VIR_HVGA_VER
   TS_MSG2133_MODEL := VIR_WVGA_VER
# RDA_CUSTOMER_DRV_TS := TS_IT7252
# RDA_CUSTOMER_DRV_TS := TS_GTP868
# RDA_CUSTOMER_DRV_TS := TS_ICN831X

# gsensor selection
# RDA_CUSTOMER_DRV_GS := GS_MMA865X
# RDA_CUSTOMER_DRV_GS := GS_MMA7660
# RDA_CUSTOMER_DRV_GS := GS_STK8312
# RDA_CUSTOMER_DRV_GS := GS_MMA845X
RDA_CUSTOMER_DRV_GS := GS_MXC622X

# light sensor selection
RDA_CUSTOMER_DRV_LS := LS_NONE

#camera sensor selection
# supported list in device/rda/driver/camera/inc/
RDA_CUSTOMER_DRV_CSB := GC2145_CSI
RDA_CUSTOMER_DRV_CSF := GC0310_CSI
#RDA_CUSTOMER_DRV_ATV := RDA5888

#DRC selection
BOARD_SUPPORT_DRC := true

# new cat
BOARD_SUPPORT_AUDIO_DYNAMIC_CALIB := true

export RDA_CUSTOMER_DRV_TS
ifeq ($(RDA_CUSTOMER_DRV_TS), TS_MSG2133)
export MSG2133_MODEL
export TS_MSG2133_MODEL
endif
export RDA_CUSTOMER_DRV_GS
export RDA_CUSTOMER_DRV_LS
export RDA_CUSTOMER_DRV_CSB
export RDA_CUSTOMER_DRV_CSF

