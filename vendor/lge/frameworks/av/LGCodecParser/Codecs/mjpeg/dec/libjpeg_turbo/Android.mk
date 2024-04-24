# Makefile for libjpeg-turbo

ifneq ($(TARGET_SIMULATOR),true)

##################################################
###                simd                        ###
##################################################
LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_CLANG := true
LOCAL_SANITIZE := signed-integer-overflow unsigned-integer-overflow

# Set ANDROID_JPEG_USE_VENUM to true to enable VeNum optimizations
ifneq ($(TARGET_IS_64_BIT),true)
ANDROID_JPEG_USE_VENUM := true
else
ANDROID_JPEG_USE_VENUM := false
endif

# Disable VeNum optimizations if they are not supported on the build target
#ifneq ($(ARCH_ARM_HAVE_VFP),true)
#ANDROID_JPEG_USE_VENUM := false
#else
#ifneq ($(ARCH_ARM_HAVE_NEON),true)
#ANDROID_JPEG_USE_VENUM := false
#endif
#endif

# From autoconf-generated Makefile
EXTRA_DIST = simd/nasm_lt.sh simd/jcclrmmx.asm simd/jcclrss2.asm simd/jdclrmmx.asm simd/jdclrss2.asm \
	simd/jdmrgmmx.asm simd/jdmrgss2.asm simd/jcclrss2-64.asm simd/jdclrss2-64.asm \
	simd/jdmrgss2-64.asm simd/CMakeLists.txt
ifneq ($(TARGET_IS_64_BIT),true)
libsimd_SOURCES_DIST = simd/jsimd_arm_neon.S \
                       asm/armv7/jdcolor-armv7.S asm/armv7/jdidct-armv7.S \
                       simd/jsimd_arm.c
endif

# or jsimd_none.c


LOCAL_SRC_FILES := $(libsimd_SOURCES_DIST)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/simd \
                    $(LOCAL_PATH)/android

ifneq ($(TARGET_IS_64_BIT),true)
LOCAL_CFLAGS := -DANDROID_JPEG_USE_VENUM
AM_CFLAGS := -march=armv7-a -mfpu=neon
AM_CCASFLAGS := -march=armv7-a -mfpu=neon
else
AM_CFLAGS := -march=armv8-a -mfpu=neon
AM_CCASFLAGS := -march=armv8-a -mfpu=neon
endif

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libsimd
LOCAL_VENDOR_MODULE := true
include $(BUILD_STATIC_LIBRARY)

######################################################
###           libjpeg_turbo.so                       ##
######################################################

include $(CLEAR_VARS)

# From autoconf-generated Makefile
libjpeg_SOURCES_DIST =  jcapimin.c jcapistd.c jccoefct.c jccolor.c \
        jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c \
        jcomapi.c jcparam.c jcphuff.c jcprepct.c jcsample.c jctrans.c \
        jdapimin.c jdapistd.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c \
        jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c \
        jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c jerror.c \
        jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c \
        jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c \
	jaricom.c jcarith.c jdarith.c \
	turbojpeg.c transupp.c jdatadst-tj.c jdatasrc-tj.c \

#possible adds jmem-android.c jmemnobs.c jmemmgr.c jmem-ashmem.c

LOCAL_SRC_FILES:= $(libjpeg_SOURCES_DIST)

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_STATIC_LIBRARIES := libsimd

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/android

LOCAL_CFLAGS := -DAVOID_TABLES  -O3 -fstrict-aliasing -fprefetch-loop-arrays  -DANDROID \
        -DANDROID_TILE_BASED_DECODE -DENABLE_ANDROID_NULL_CONVERT
ifneq ($(TARGET_IS_64_BIT),true)
LOCAL_CFLAGS += -DANDROID_JPEG_USE_VENUM
else
LOCAL_CFLAGS += -DSIMD_UNSUPPORTED
endif

#-DANDROID_TILE_BASED_DECODE -DUSE_ANDROID_ASHMEM

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_STATIC_LIBRARY)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libjpeg_turbo
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)
endif
