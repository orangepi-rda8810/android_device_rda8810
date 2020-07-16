
# for rdamicro internal proprietories modules: only support package module names

# for vivante gpu
ifeq ($(BOARD_USES_GPU_VIVANTE),true)
GPUCORE := \
        galcore.ko \
        libGAL \
        libGLSLC \
        libEGL_VIVANTE \
        libGLESv1_CM_VIVANTE \
        libGLESv2_VIVANTE \
        gralloc.$(TARGET_PLATFORM)
endif

# for mali gpu
ifeq ($(BOARD_USES_GPU_MALI),true)
GPUCORE := \
        mali.ko \
        libGLES_mali \
        gralloc.$(TARGET_PLATFORM)
endif

# for chipsnmedia vpu
ifeq ($(BOARD_USES_VPU_CODA),true)
VPUCORE := \
        vpu.ko \
        libstagefrighthw \
        libomxvpu \
        libomxjpu \
        libomxil-bellagio \
        libvpu \
        libtheoraparser \
        omxregister-bellagio \
        vpurun \
	.omxregister
endif

# for chipsnmedia jpu
ifeq ($(BOARD_USES_JPU_CODA),true)
JPUCORE := \
        jpu.ko \
        jpurun \
        libjpu
endif

ifeq ($(BOARD_USES_VOC),true)
VOCCORE := \
	voc.ko \
	libvoc \
	vocrun
endif

ifeq ($(BOARD_USES_CAM),true)
CAM_HAL := \
	camera.$(TARGET_PLATFORM)
endif

PRODUCT_PACKAGES := $(GPUCORE) $(VPUCORE) $(JPUCORE) $(VOCCORE) $(CAM_HAL)

