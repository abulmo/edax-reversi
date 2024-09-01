LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := aEdax # should be renamed to lib..aEdax..so afterwords
LOCAL_CFLAGS += -DUNICODE
LOCAL_SRC_FILES := all.c board_sse.c.neon eval_sse.c.neon flip_neon_bitscan.c.neon android/cpu-features.c
LOCAL_ARM_NEON := false
# cmd-strip :=
include $(BUILD_EXECUTABLE)
