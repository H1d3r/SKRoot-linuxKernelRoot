LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := web_server
LOCAL_SRC_FILES := \
../web_server.cpp \
../cJSON.cpp

SKROOT_BOX := $(LOCAL_PATH)/../../../../../skroot_box
KERNEL_MODULE_KIT := $(SKROOT_BOX)/../../../testModule/kernel_module_kit

LOCAL_C_INCLUDES  += $(SKROOT_BOX)
LOCAL_C_INCLUDES  += $(SKROOT_BOX)/include
LOCAL_C_INCLUDES  += $(SKROOT_BOX)/src/jni/
LOCAL_LDFLAGS  += $(SKROOT_BOX)/lib/libskroot_box_static.a
LOCAL_LDFLAGS += $(KERNEL_MODULE_KIT)/lib/libkernel_module_kit_static.a

include $(LOCAL_PATH)/build_macros.mk
include $(LOCAL_PATH)/civetweb.mk
include $(BUILD_EXECUTABLE)
