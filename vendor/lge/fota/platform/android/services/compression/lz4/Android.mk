LOCAL_PATH:= $(call my-dir)

common_SRC_FILES:=  \
	lz4.c \
	lz4hc.c \
	bench.c \
	lz4demo.c


# static library
# =====================================================

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= $(common_SRC_FILES)

LOCAL_MODULE := lz4elix

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libcutils libc

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/sbin

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
