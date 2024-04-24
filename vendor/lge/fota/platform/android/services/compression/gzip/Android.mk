LOCAL_PATH:= $(call my-dir)

common_SRC_FILES:=  \
	bits.c \
	crypt.c \
	deflate.c \
	getopt.c \
	gzip.c \
	inflate.c \
	lzw.c \
	trees.c \
	unlzh.c \
	unlzw.c \
	unpack.c \
	unzip.c \
	util.c \
	zip.c

common_C_INCLUDES +=  $(LOCAL_PATH)/include

# static library
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= $(common_SRC_FILES)
LOCAL_C_INCLUDES:= $(common_C_INCLUDES)

ifdef TARGET_2ND_ARCH
LGUA_TARGET_ARCH := 64
else
LGUA_TARGET_ARCH := 32
endif

LOCAL_MODULE := gzipelix

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_STATIC_LIBRARIES := libcutils libc

LOCAL_PREBUILT_OBJ_FILES := include/libgzip_$(LGUA_TARGET_ARCH).a

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/sbin

LOCAL_CFLAGS := -I $(LOCAL_PATH)/include

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
