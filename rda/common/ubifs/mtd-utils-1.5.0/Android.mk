# Copyright 2005 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
# libubigen.a
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ubi-utils/libubigen.c

LOCAL_CFLAGS =   -O2 -Wall

LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls \
	-Wnested-externs -Winline

LOCAL_LDLIBS+=

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)/ubi-utils/include

LOCAL_MODULE := libubigen
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_STATIC_LIBRARY)

# libubi.a
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ubi-utils/libubi.c

LOCAL_CFLAGS =   -O2 -Wall

LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls \
	-Wnested-externs -Winline

LOCAL_LDLIBS+=

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)/ubi-utils/include

LOCAL_MODULE := libubi
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_STATIC_LIBRARY)

# libscan
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ubi-utils/libscan.c

LOCAL_CFLAGS =   -O2 -Wall

LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls \
	-Wnested-externs -Winline

LOCAL_LDLIBS+=

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)/ubi-utils/include

LOCAL_MODULE := libscan
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_STATIC_LIBRARY)

# libscan
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	lib/libmtd.c \
	lib/libmtd_legacy.c \
	lib/libcrc32.c \
	lib/libfec.c

LOCAL_CFLAGS =   -O2 -Wall

LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls \
	-Wnested-externs -Winline

LOCAL_LDLIBS+=

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)/ubi-utils/include

LOCAL_MODULE := libmtd
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_STATIC_LIBRARY)

# libiniparser
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ubi-utils/libiniparser.c \

LOCAL_CFLAGS =   -O2 -Wall

LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls \
	-Wnested-externs -Winline

LOCAL_LDLIBS+=

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)/ubi-utils/include

LOCAL_MODULE := libiniparser
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_STATIC_LIBRARY)

# mkubifs
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mkfs.ubifs/mkfs.ubifs.c \
	mkfs.ubifs/crc16.c \
	mkfs.ubifs/lpt.c \
	mkfs.ubifs/compr.c \
	mkfs.ubifs/devtable.c \
	mkfs.ubifs/hashtable/hashtable.c \
	mkfs.ubifs/hashtable/hashtable_itr.c

LOCAL_CFLAGS =   -O2 -Wall -DANDROID
LOCAL_CFLAGS += -DANDROID_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls \
	-Wnested-externs -Winline
LOCAL_SHARED_LIBRARIES := liblzo2_host libubi_uuid_host
ifeq (,$(findstring $(PLATFORM_SDK_VERSION), "19" "20" "21" "22"))
LOCAL_SHARED_LIBRARIES += libcutils
endif
LOCAL_LDFLAGS += -lm -lz

LOCAL_C_INCLUDES += $(LOCAL_PATH)/.. $(LOCAL_PATH)/include $(LOCAL_PATH)/ubi-utils/include $(LOCAL_PATH)/../../lzo/include

LOCAL_STATIC_LIBRARIES := libubigen libubi libscan libmtd libselinux
# diff name from mkfs.ubifs on pc
LOCAL_MODULE := mkubifs
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_EXECUTABLE)

# ubinize 
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ubi-utils/ubinize.c \
	ubi-utils/dictionary.c \
	ubi-utils/ubiutils-common.c

LOCAL_CFLAGS =   -O2 -Wall

LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls \
	-Wnested-externs -Winline

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../external/e2fsprogs/lib $(LOCAL_PATH)/include $(LOCAL_PATH)/ubi-utils/include $(LOCAL_PATH)/../../lzo/include

LOCAL_STATIC_LIBRARIES := libubigen libubi libscan libmtd libiniparser 

LOCAL_MODULE := ubinize
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_EXECUTABLE)
