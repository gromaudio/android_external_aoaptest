LOCAL_PATH := $(call my-dir)


#include $(CLEAR_VARS)
#LOCAL_MODULE          := libssl
#LOCAL_MODULE_SUFFIX   := .a
#LOCAL_MODULE_CLASS    := STATIC_LIBRARIES
#LOCAL_MODULE_TAGS     := eng
#LOCAL_PRELINK_MODULE  := false
#LOCAL_MODULE_PATH     := system/lib
#LOCAL_SRC_FILES       := src/android_auto/libs/libssl.a
#include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE          := libcrypto
LOCAL_MODULE_SUFFIX   := .a
LOCAL_MODULE_CLASS    := STATIC_LIBRARIES
LOCAL_MODULE_TAGS     := eng
LOCAL_PRELINK_MODULE  := false
LOCAL_MODULE_PATH     := system/lib
LOCAL_SRC_FILES       := src/android_auto/libs/libcrypto.a
include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE          := libusbnok
#LOCAL_MODULE_SUFFIX   := .a
#LOCAL_MODULE_CLASS    := STATIC_LIBRARIES
#LOCAL_MODULE_TAGS     := eng
#LOCAL_PRELINK_MODULE  := false
#LOCAL_MODULE_PATH     := system/lib
#LOCAL_SRC_FILES       := src/android_auto/libs/libusbnok.a
#include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE      := aoap_test
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS      := -Wno-return-type \
                     -Wno-parentheses \
                     -Wno-missing-braces \
                     -Wno-sign-compare \
                     -Wno-type-limits

LOCAL_SRC_FILES:= \
        main_test.cpp \
        AndroidAuto.cpp \
        AndroidAutoTouch.cpp \
        utils/Utils.cpp \
        src/common.c \
        src/os.c \
        src/interface/android/android_commands.c \
        src/interface/android/android_browser.c \
        src/intercom/usbhost_bulk.c \
        src/android_auto/hu.c \
        src/android_auto/hu_uti.c \
        src/android_auto/hu_aap.c \
        src/android_auto/hu_usb.c \
        src/android_auto/hu_ssl.c \
        src/android_auto/hu_aad.c

LOCAL_C_INCLUDES += \
        bionic \
        bionic/libstdc++/include \
        external/stlport/stlport \
        external/tinyalsa/include \
        external/libusb/libusb \
        external/libusb \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/src \
        $(LOCAL_PATH)/src/android_auto \
        $(LOCAL_PATH)/utils \
        system/vbased/uart-service \
        system/vbased/common

LOCAL_STATIC_LIBRARIES := \
        libcrypto \
        libvbased

LOCAL_SHARED_LIBRARIES := \
        libc \
        libdl \
        libcutils \
        libutils \
        libstlport \
        libmedia \
        libtinyalsa \
        libmedia \
        libbinder \
        libgui \
        libusb \
        libusbhost \
        libui \
        libstagefright_foundation \
        libstagefright \
        libg2d \
        libssl

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_MODULE      := aauto
LOCAL_CFLAGS      := -Wno-return-type \
                     -Wno-parentheses \
                     -Wno-missing-braces \
                     -Wno-sign-compare \
                     -Wno-type-limits

LOCAL_SRC_FILES:= \
        main_auto.cpp \
        AndroidAuto.cpp \
        AndroidAutoTouch.cpp \
        utils/Utils.cpp \
        src/intercom/usbhost_bulk.c \
        src/android_auto/hu.c \
        src/android_auto/hu_uti.c \
        src/android_auto/hu_aap.c \
        src/android_auto/hu_usb.c \
        src/android_auto/hu_ssl.c \
        src/android_auto/hu_aad.c

LOCAL_C_INCLUDES += \
        bionic \
        bionic/libstdc++/include \
        external/stlport/stlport \
        external/tinyalsa/include \
        external/libusb/libusb \
        external/libusb \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/src \
        $(LOCAL_PATH)/src/android_auto \
        $(LOCAL_PATH)/utils \
        system/vbased/uart-service \
        system/vbased/common

LOCAL_STATIC_LIBRARIES := \
        libcrypto \
        libvbased

LOCAL_SHARED_LIBRARIES := \
        libc \
        libdl \
        libcutils \
        libutils \
        libstlport \
        libmedia \
        libtinyalsa \
        libmedia \
        libbinder \
        libgui \
        libusb \
        libusbhost \
        libui \
        libstagefright_foundation \
        libstagefright \
        libg2d \
        libssl

include $(BUILD_EXECUTABLE)
