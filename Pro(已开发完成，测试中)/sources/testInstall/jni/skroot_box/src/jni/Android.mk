LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := skroot_box_static

LOCAL_PIC := true

SKROOT_BOX := $(LOCAL_PATH)/../../../skroot_box
KERNEL_MODULE_KIT := $(SKROOT_BOX)/../../../testModule/kernel_module_kit

LIBWEB_SERVER_LOADER_DATA  := $(SKROOT_BOX)/src/jni/lib_web_server_loader/lib_web_server_loader_data.h
ifneq ($(wildcard $(LIBWEB_SERVER_LOADER_DATA)),)
  LOCAL_CPPFLAGS += -include $(LIBWEB_SERVER_LOADER_DATA)
endif

LOCAL_C_INCLUDES += $(KERNEL_MODULE_KIT)/include

include $(SKROOT_BOX)/src/jni/skroot_box_src_file.mk
include $(LOCAL_PATH)/build_macros_static.mk

include $(BUILD_STATIC_LIBRARY)