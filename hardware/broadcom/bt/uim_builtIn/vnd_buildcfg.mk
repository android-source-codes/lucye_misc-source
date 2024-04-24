#BT_S : [CONBT-2289] LGC_BT_COMMON_IMP_V4L2_REFACTORING_HEADER_FILE  20150910, [START]

SRC := $(call my-dir)/include/$(addprefix vnd_v4l2_, $(addsuffix .txt,$(basename $(TARGET_DEVICE))))
ifeq ($(BOARD_HAVE_BLUETOOTH_BCM_SW),)
ifeq (,$(wildcard $(SRC)))
# configuration file does not exist. Use default one
SRC := $(call my-dir)/include/vnd_v4l2_generic.txt
endif
endif
GEN := $(call intermediates-dir-for,EXECUTABLES,$(LOCAL_MODULE))/include/vnd_v4l2cfg.h
TOOL := $(LOCAL_PATH)/gen-buildcfg.sh
$(warning BOARD_HAVE_BLUETOOTH_BCM --- $(BOARD_HAVE_BLUETOOTH_BCM))
$(warning BOARD_HAVE_BLUETOOTH_BCM_SW --- $(BOARD_HAVE_BLUETOOTH_BCM_SW))
$(warning BRCM_BLUEDROID_VER --- $(BRCM_BLUEDROID_VER))
$(warning TARGET_PRODUCT --- $(TARGET_PRODUCT))
$(warning SRC --- $(SRC))

$(GEN): PRIVATE_PATH := $(call my-dir)
$(GEN): PRIVATE_CUSTOM_TOOL = $(TOOL) $< $@
$(GEN): $(SRC)  $(TOOL)
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)
LOCAL_C_INCLUDES += $(dir $(GEN))
#BT_E : [CONBT-2289] LGC_BT_COMMON_IMP_V4L2_REFACTORING_HEADER_FILE  20150910, [END]

