/*****************************************************************************
 *   usbhost_bulk.c: Bulk interface for libusb connections.
 *
 *   Copyright(C) 2015, X-mediatech
 *   All rights reserved.
 *
 *   Authors: Ivan Zaitsev <ivan.zaitsev@gmail.com>
 *
******************************************************************************/
#define LOG_TAG "AOAP"

#include "log/log.h"
#include "common.h"
#include "libusb/libusb.h"
#include "intercom/usbhost_bulk.h"
#include "interface/android/android_accessory.h"

//-----------------------------------------------------------------------------
#define AOAP_VID                        0x18d1
#define AOAP_INTERFACE                  0
#define AOAP_IN_EP                      0x81
#define AOAP_OUT_EP                     0x02

//-----------------------------------------------------------------------------
const uint16_t aSupportedPIDs[] = { 0x2d02, 0x2d03, 0x2d04, 0x2d05 };
const unsigned char pCapMsg[] = { MESSAGE_TYPE_CAPABILITIES,  0x00, 0x02, CAP_VERSION, AALINQ_PROTOCOL_VER };
libusb_device_handle *dev_handle;

const char* libusb_error_name(int errcode);

//-----------------------------------------------------------------------------
//                                      Bulk Drive Insert
//-----------------------------------------------------------------------------
signed char Bulk_DriveInsert( void )
{
  unsigned int  i;
  int           r,
                active_congif;

  r = libusb_init(NULL);
  if(r < 0)
    return ERR_INIT_FAILED;

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
    return ERR_ENUM_FAILED;
  }

  r = libusb_get_configuration(dev_handle, &active_congif);
  if(r < 0)
  {
	  ALOGE("Get active configuration error: %d %s\n", r, libusb_error_name(r));
    libusb_close(dev_handle);
    libusb_exit(NULL);
    return ERR_ENUM_FAILED;
  }

  ALOGE("Active configuration: %d\n", active_congif);

  if(1 == libusb_kernel_driver_active(dev_handle, AOAP_INTERFACE))
  {
	  ALOGE("Kernel driver active on interface %d.\n", AOAP_INTERFACE);
    r = libusb_detach_kernel_driver(dev_handle, AOAP_INTERFACE);
    if(r < 0)
    	ALOGE("Detach kernel driver error: %d %s\n", r, libusb_error_name(r));
  }

  r = libusb_claim_interface(dev_handle, AOAP_INTERFACE);
  if(r < 0)
  {
	  ALOGE("Claim interface error: %d %s\n", r, libusb_error_name(r));
    libusb_close(dev_handle);
    libusb_exit(NULL);
    return ERR_ENUM_FAILED;
  }

  if(OK != Bulk_OUT((volatile unsigned char*)pCapMsg,
                    3 + ReadBE16U( (unsigned char*)( pCapMsg + 1 ) )))
  {
	  ALOGD("First Bulk_OUT error");
    libusb_release_interface(dev_handle, AOAP_INTERFACE);
    libusb_close(dev_handle);
    libusb_exit(NULL);
    return ERR_ENUM_FAILED;
  }
  return OK;
}

//-----------------------------------------------------------------------------
//                                      BULK OUT TRANSFER
//-----------------------------------------------------------------------------
signed char Bulk_OUT( volatile unsigned char *aBuff, unsigned short iSize )
{
  int bytesTransfered, r;

  r = libusb_bulk_transfer(dev_handle,
                           AOAP_OUT_EP,
                           (unsigned char*)aBuff,
                           iSize,
                           &bytesTransfered,
                           1000);
  if(r < 0 || (iSize != bytesTransfered))
  {
    ALOGD("OUT error: %d %s\n", r, libusb_error_name(r));
    return ERR_TD_FAIL;
  }
  return OK;
}

//-----------------------------------------------------------------------------
//                                      BULK IN TRANSFER
//-----------------------------------------------------------------------------
signed char Bulk_IN( volatile unsigned char *aBuff, unsigned short iSize, unsigned int *piBytesRead )
{
  int r;

  r = libusb_bulk_transfer(dev_handle,
                           AOAP_IN_EP,
                           (unsigned char*)aBuff,
                           iSize,
                           (int*)piBytesRead,
                           100);
  if(r < 0)
  {
	ALOGD("IN error: %d %s\n", r, libusb_error_name(r));
    return ERR_TD_FAIL;
  }
  return OK;
}
