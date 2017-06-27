/*
 * usbhost_hid.c
 *
 *  Created on: Jun 22, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#define LOG_TAG "AOAP"

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "log/log.h"
#include "intercom/usbhost_hid.h"
#include "libusb/libusb.h"

#define AOAP_VID            0x18d1
#define HID_MAX_PACKET_SIZE 64
#define HID_DEVICE_ID       1
#define AOAP_INTERFACE      1 //mId=1,mAlternateSetting=1

#define ACCESSORY_REGISTER_HID         54
#define ACCESSORY_UNREGISTER_HID       55
#define ACCESSORY_SET_HID_REPORT_DESC  56
#define ACCESSORY_SEND_HID_EVENT       57

const unsigned char  ReportDescriptor[] = {
	0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x85, 0x01, 0x95, 0x03, 0x75, 0x01,
	0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
	0x81, 0x01, 0x75, 0x08, 0x95, 0x02, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x80, 0x25, 0x7F,
	0x81, 0x06, 0xC0, 0xA1, 0x00, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x38, 0x15, 0x81, 0x25,
	0x7F, 0x81, 0x06, 0xC0, 0xC0, 0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x02, 0x19, 0x00, 0x2A,
	0x3C, 0x02, 0x15, 0x00, 0x26, 0x3C, 0x02, 0x95, 0x01, 0x75, 0x10, 0x81, 0x00, 0xC0, 0x06, 0xBC,
	0xFF, 0x09, 0x88, 0xA1, 0x01, 0x85, 0x03, 0x19, 0x01, 0x29, 0xFF, 0x15, 0x01, 0x26, 0xFF, 0x00,
	0x95, 0x01, 0x75, 0x08, 0x81, 0x00, 0xC0, 0x06, 0x01, 0xFF, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x04,
	0x95, 0x01, 0x75, 0x08, 0x15, 0x01, 0x25, 0x0A, 0x09, 0x20, 0xB1, 0x03, 0x25, 0x4F, 0x09, 0x21,
	0xB1, 0x03, 0x25, 0x30, 0x09, 0x22, 0xB1, 0x03, 0x75, 0x10, 0x09, 0x23, 0xB1, 0x03, 0x09, 0x24,
	0xB1, 0x03, 0xC0, 0x06, 0x01, 0xFF, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x05, 0x95, 0x01, 0x75, 0x08,
	0x15, 0x01, 0x25, 0x0A, 0x09, 0x20, 0xB1, 0x03, 0x25, 0x4F, 0x09, 0x21, 0xB1, 0x03, 0x25, 0x30,
	0x09, 0x22, 0xB1, 0x03, 0x75, 0x10, 0x09, 0x23, 0xB1, 0x03, 0x09, 0x24, 0xB1, 0x03, 0xC0
};

const char* libusb_error_name(int errcode);

const uint16_t aSupportedPIDs[] = { 0x2d02, 0x2d03, 0x2d04, 0x2d05 };
struct libusb_device_handle *dev_handle;

/*
*******************************************************************************
* Init HID
*******************************************************************************
*/
int hid_init( void )
{
  unsigned int            r, i,
  	  	  	  	  	  	  iSize;
  unsigned short          iBytesToSend;
  unsigned char  		  pHostBuffer[HID_MAX_PACKET_SIZE];
  libusb_device *dev;

  r = libusb_init(NULL);
  if(r < 0) {
    ALOGE("libusb_init() error: %d %s\n", r, libusb_error_name(r));
    return ERR_INIT_FAILED;
  }

  dev_handle = NULL;
  for(i = 0; i < NELEMS(aSupportedPIDs); i++)
  {
    dev_handle = libusb_open_device_with_vid_pid(NULL, AOAP_VID, aSupportedPIDs[ i ]);
    if(dev_handle != NULL)
    {
      ALOGE("Accessory device found(%04x:%04x).\n", AOAP_VID, aSupportedPIDs[ i ]);
      break;
    }
  }

  if(dev_handle == NULL)
  {
	  ALOGE("No accessory device found.\n");
    libusb_exit(NULL);
    return ERR_INIT_FAILED;
  }

  //HID registration
  r = libusb_control_transfer(dev_handle,
		  LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE ,
		  ACCESSORY_REGISTER_HID,
		  HID_DEVICE_ID,
		  sizeof( ReportDescriptor ),
		  NULL, 0,
		  100);
  if (r < 0) {
	  ALOGE("Register HID error: %d %s\n", r, libusb_error_name(r));
	  libusb_close(dev_handle);
	  libusb_exit(NULL);
	  return ERR_INIT_FAILED;
  }

  iSize = 0;
  while( iSize < sizeof( ReportDescriptor ) )
  {
    iBytesToSend = GET_MIN_VALUE( sizeof( ReportDescriptor ) - iSize, HID_MAX_PACKET_SIZE );
    memcpy( (void*)pHostBuffer, ReportDescriptor + iSize, iBytesToSend );
    r = libusb_control_transfer(dev_handle,
    		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
            ACCESSORY_SET_HID_REPORT_DESC,
            HID_DEVICE_ID,
            iSize,
            pHostBuffer,
            iBytesToSend,
            100);
    if (r < 0) {
  	    ALOGE("SET_HID_REPORT_DESC error: %d %s\n", r, libusb_error_name(r));
  	    libusb_close(dev_handle);
  	    libusb_exit(NULL);
  	    return ERR_INIT_FAILED;
    }

    iSize += iBytesToSend;
  }

  return ( iSize == sizeof( ReportDescriptor ) ) ? OK : ERR_INIT_FAILED;
}

/*
*******************************************************************************
* Send HID command to device.
*******************************************************************************
*/
void hid_send( HID_COMMAND cmd )
{
  int r;
  unsigned char  HidReport[ 3 ];

  HidReport[ 0 ] = 0x02;
  HidReport[ 1 ] = cmd;
  HidReport[ 2 ] = 0x00;
  r = libusb_control_transfer(dev_handle,
  		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
  		ACCESSORY_SEND_HID_EVENT,
        HID_DEVICE_ID,
        0,
        HidReport,
        sizeof( HidReport ),
        100);
  if (r < 0) {
	  ALOGE("ACCESSORY_SEND_HID_EVENT(%d) error: %d %s\n", cmd, r, libusb_error_name(r));
  }

  usleep(50*1000);

  HidReport[ 0 ] = 0x02;
  HidReport[ 1 ] = 0x00;
  HidReport[ 2 ] = 0x00;
  r = libusb_control_transfer(dev_handle,
  		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
  		ACCESSORY_SEND_HID_EVENT,
        HID_DEVICE_ID,
        0,
        HidReport,
        sizeof( HidReport ),
        100);

  if (r < 0) {
	  ALOGE("ACCESSORY_SEND_HID_EVENT(0) error: %d %s\n", r, libusb_error_name(r));
  }
}

/*
*******************************************************************************
* Enable linux USB Audio driver
*******************************************************************************
*/
int enable_iso_driver(void)
{
	int r;
	if (0 == libusb_kernel_driver_active(dev_handle, AOAP_INTERFACE)) {
		ALOGD("Kernel driver disabled on interface %d.\n", AOAP_INTERFACE);
		r = libusb_attach_kernel_driver(dev_handle, AOAP_INTERFACE);
		if(r < 0){
			ALOGE("Attach kernel driver error: %d %s\n", r, libusb_error_name(r));
			return ERR_INIT_FAILED;
		}
	}
	ALOGD("Kernel driver now active on interface %d.\n", AOAP_INTERFACE);
	return OK;
}

/*
*******************************************************************************
* Disable linux USB Audio driver
*******************************************************************************
*/
int disable_iso_driver(void)
{
	int r;
	if (1 == libusb_kernel_driver_active(dev_handle, AOAP_INTERFACE)) {
		ALOGD("Kernel driver active on interface %d.\n", AOAP_INTERFACE);
		r = libusb_detach_kernel_driver(dev_handle, AOAP_INTERFACE);
		if(r < 0){
			ALOGE("Detach kernel driver error: %d %s\n", r, libusb_error_name(r));
			return ERR_INIT_FAILED;
		}
	}
	ALOGD("Kernel driver now disabled on interface %d.\n", AOAP_INTERFACE);
	return OK;
}
