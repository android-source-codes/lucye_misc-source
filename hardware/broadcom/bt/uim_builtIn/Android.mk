ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
ifeq ($(BLUEDROID_ENABLE_V4L2_BUILT_IN),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

#
# UIM Application
#

LOCAL_SRC_FILES:= \
    uim.cc \
    upio.cc \
    brcm_hci_dump.cc \
    btsnoop.cc \
    utils.cc

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/include

LOCAL_CFLAGS:= -g -c -W -Wall -O2 -D_POSIX_SOURCE -DUIM_DEBUG

ifeq ($(BLUEDROID_ENABLE_V4L2),true)
LOCAL_CFLAGS += -DBLUEDROID_ENABLE_V4L2
endif

LOCAL_SHARED_LIBRARIES:= libnetutils libcutils liblog

#BT_S : [ZTWOGFOURM-1302] workaround for sepolicy issue
LOCAL_SHARED_LIBRARIES += libselinux
#BT_E : [ZTWOGFOURM-1302] workaround for sepolicy issue

LOCAL_MODULE := brcm-uim-sysfs
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := broadcom
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
#BT_S : [CONBT-2289] LGC_BT_COMMON_IMP_V4L2_REFACTORING_HEADER_FILE  20150910, [START]
include $(LOCAL_PATH)/vnd_buildcfg.mk
#BT_E : [CONBT-2289] LGC_BT_COMMON_IMP_V4L2_REFACTORING_HEADER_FILE  20150910, [END]
include $(BUILD_EXECUTABLE)

endif
endif