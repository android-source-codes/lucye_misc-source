ifneq ($(USE_CLAT_AOSP),true)
# LGP_DATA_PERFORMANCE_CLAT_BY_QC
# LG_CLAT - (dataformat_dl_gro_enabled=0), netmgr_config.xml
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=clatd.c dump.c checksum.c translate.c icmp.c ipv4.c ipv6.c config.c dns64.c logging.c getaddr.c netlink_callbacks.c netlink_msg.c setif.c mtu.c tun.c

LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
# Bug: http://b/33566695
LOCAL_CFLAGS += -Wno-address-of-packed-member
LOCAL_C_INCLUDES := external/libnl/include bionic/libc/dns/include
LOCAL_C_INCLUDES += vendor/lge/system/dsqn/libpatchcodeid
# 2019-03-19 yunsik.lee@lge.com LGP_DATA_CLATD_PRIVATE_DNS_BYPASS [START]
LOCAL_C_INCLUDES += system/netd/include
# 2019-03-19 yunsik.lee@lge.com LGP_DATA_CLATD_PRIVATE_DNS_BYPASS [END]
LOCAL_STATIC_LIBRARIES := libnl
LOCAL_SHARED_LIBRARIES := libcutils liblog libnetutils
LOCAL_SHARED_LIBRARIES += libpatchcodeid

# The clat daemon.
LOCAL_MODULE := clatd

include $(BUILD_EXECUTABLE)


# The configuration file.
include $(CLEAR_VARS)

LOCAL_MODULE := clatd.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc
LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)


# Unit tests.
include $(CLEAR_VARS)

LOCAL_MODULE := clatd_test
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
# Bug: http://b/33566695
LOCAL_CFLAGS += -Wno-address-of-packed-member
LOCAL_SRC_FILES := clatd_test.cpp dump.c checksum.c translate.c icmp.c ipv4.c ipv6.c logging.c config.c tun.c
# Bug: http://b/33566695
LOCAL_CFLAGS += -Wno-address-of-packed-member
LOCAL_MODULE_TAGS := eng tests
LOCAL_SHARED_LIBRARIES := liblog libnetutils

include $(BUILD_NATIVE_TEST)

# Microbenchmark.
include $(CLEAR_VARS)

LOCAL_MODULE := clatd_microbenchmark
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
LOCAL_SRC_FILES := clatd_microbenchmark.c checksum.c tun.c
LOCAL_MODULE_TAGS := eng tests

include $(BUILD_NATIVE_TEST)

endif # LGP_DATA_PERFORMANCE_CLAT_BY_QC
