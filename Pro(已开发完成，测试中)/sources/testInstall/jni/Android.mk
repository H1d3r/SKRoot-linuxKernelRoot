LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := testInstall
LOCAL_SRC_FILES := testInstall.cpp

SKROOT_BOX := $(LOCAL_PATH)/skroot_box
KERNEL_MODULE_KIT := $(SKROOT_BOX)/../../../testModule/kernel_module_kit

LOCAL_C_INCLUDES += $(SKROOT_BOX)/include
LOCAL_C_INCLUDES += $(KERNEL_MODULE_KIT)/include

LOCAL_LDFLAGS += $(SKROOT_BOX)/lib/libskroot_box_static.a
LOCAL_LDFLAGS += $(KERNEL_MODULE_KIT)/lib/libkernel_module_kit_static.a
LOCAL_LDLIBS += -lz
include $(LOCAL_PATH)/build_macros.mk
include $(BUILD_EXECUTABLE)
