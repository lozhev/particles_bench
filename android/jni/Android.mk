TARGET_PLATFORM := android-9

LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_MODULE    := freeglut
#LOCAL_SRC_FILES := ../../../freeglut/build-and/obj/local/$(TARGET_ARCH_ABI)/libfreeglut.a
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := native

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

#libs files
#LOCAL_SRC_FILES += android_native_app_glue.c ../../src/Image.c ../../src/App.c ../../src/ShaderMan.c ../../src/and_main.c

#LOCAL_ARM_MODE := arm
#COMMON_CFLAGS := -DFREEGLUT_GLES -DFREEGLUT_STATIC

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS := -fstack-protector -fno-rtti -fno-omit-frame-pointer -fno-exceptions -fstack-check $(COMMON_CFLAGS)
else
	LOCAL_CFLAGS := -fstack-check $(COMMON_CFLAGS)
endif

LOCAL_LDLIBS := -landroid -llog -lGLESv2 -lEGL -lGLESv1_CM

#LOCAL_C_INCLUDES += ../../freeglut/include

#LOCAL_STATIC_LIBRARIES := freeglut

include $(BUILD_SHARED_LIBRARY)
