LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OPENCV_INSTALL_MODULES:=on

include /Users/pushkar/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk

LOCAL_MODULE    := edge_detector_native
LOCAL_SRC_FILES := jni_part.cpp
LOCAL_LDLIBS +=  -llog -ldl
LOCAL_LDFLAGS += -latomic
include $(BUILD_SHARED_LIBRARY)
