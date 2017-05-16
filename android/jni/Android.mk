TARGET_PLATFORM := android-9

LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_MODULE    := freeglut
#LOCAL_SRC_FILES := ../../../freeglut/build-and/obj/local/$(TARGET_ARCH_ABI)/libfreeglut.a
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := native

#LOCAL_C_INCLUDES += ../../freeglut/include

LOCAL_SRC_FILES +=  ../../and_main.c \
					../../main.c \
					../../pch.c \
					../../test1.c \
					../../test2.c \
					../../test3.c \
					../../test4.c \
					../../test5.c \
					../../test6.c \
					../../test7.c \
					../../test_static.c \
					android_native_app_glue.c

#COMMON_CFLAGS := -DFREEGLUT_GLES -DFREEGLUT_STATIC
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
COMMON_CFLAGS := -Wall 
#-Werror

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS := -fstack-protector $(COMMON_CFLAGS)
else
	LOCAL_CFLAGS := $(COMMON_CFLAGS)
endif

LOCAL_LDLIBS := -landroid -llog -lGLESv2 -lEGL -lGLESv1_CM 
#-lGLESv3
#LOCAL_LDFLAGS := --no-warn

#LOCAL_STATIC_LIBRARIES := freeglut

include $(BUILD_SHARED_LIBRARY)
