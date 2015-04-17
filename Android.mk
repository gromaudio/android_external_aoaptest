LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= main.cpp \
                  aoap_stream.cpp \
                  messageQueue.cpp

LOCAL_MODULE:= aoap_test

LOCAL_CFLAGS :=

LOCAL_C_INCLUDES += \
        bionic \
        bionic/libstdc++/include \
        external/stlport/stlport \
        external/libusb

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libutils \
        libstlport \
        libusb

LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)
