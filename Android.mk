LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= main.cpp \
                  pcm_stream.cpp \
                  messageQueue.cpp \
                  src/common.c \
                  src/os.c \
                  src/interface/android/android_commands.c \
                  src/interface/android/android_browser.c \
                  src/intercom/usbhost_bulk.c \

LOCAL_MODULE:= aoap_test

LOCAL_CFLAGS :=

LOCAL_C_INCLUDES += \
        bionic \
        bionic/libstdc++/include \
        external/stlport/stlport \
        external/tinyalsa/include \
        external/libusb \
        $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES := \
        libc \
        libcutils \
        libutils \
        libstlport \
        libmedia \
        libtinyalsa \
        libusb

LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)
