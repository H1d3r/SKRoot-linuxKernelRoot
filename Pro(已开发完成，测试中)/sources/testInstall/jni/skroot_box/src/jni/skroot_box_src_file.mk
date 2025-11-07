SKROOT_BOX_SRCS := \
    $(SKROOT_BOX)/src/jni/skroot_box_command.cpp \
    $(SKROOT_BOX)/src/jni/skroot_box_exec_process.cpp \
    $(SKROOT_BOX)/src/jni/skroot_box_fork_helper.cpp \
    $(SKROOT_BOX)/src/jni/skroot_box_test.cpp \
    $(SKROOT_BOX)/src/jni/skroot_box_parasite_app.cpp \
    $(SKROOT_BOX)/src/jni/skroot_box_parasite_patch_elf.cpp \
    $(SKROOT_BOX)/src/jni/skroot_box_process_cmdline_utils.cpp
LOCAL_SRC_FILES += $(SKROOT_BOX_SRCS)
LOCAL_C_INCLUDES += $(SKROOT_BOX)
LOCAL_C_INCLUDES += $(SKROOT_BOX)/include
