LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= aoap_test.cpp

LOCAL_MODULE:= aoap_test

LOCAL_CFLAGS :=

LOCAL_C_INCLUDES += \
        external/libusb

LOCAL_SHARED_LIBRARIES := \
        libusb

LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)
