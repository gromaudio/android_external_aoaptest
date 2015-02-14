/*
 * libusb example program to list devices on the bus
 * Copyright (C) 2007 Daniel Drake <dsd@gentoo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include "libusb/libusb.h"

//-----------------------------------------------------------------------------
#define ACCESSORY_GET_PROTOCOL          51
#define ACCESSORY_SEND_STRING           52
#define ACCESSORY_START                 53
#define ACCESSORY_REGISTER_HID          54
#define ACCESSORY_UNREGISTER_HID        55
#define ACCESSORY_SET_HID_REPORT_DESC   56
#define ACCESSORY_SEND_HID_EVENT        57
#define ACCESSORY_AUDIO                 58

#define ACCESSORY_STRING_MANUFACTURER   "Gromaudio"
#define ACCESSORY_STRING_MODEL          "GROMLinQ"
#define ACCESSORY_STRING_DESCRIPTION    "Accessory Linq"
#define ACCESSORY_STRING_VERSION        "0.9"
#define ACCESSORY_STRING_URL            "http://www.gromaudio.com/aalinq"
#define ACCESSORY_STRING_SERIAL         "0000000012345678"

//-----------------------------------------------------------------------------
static void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}

		printf("%04x:%04x (bus %d, device %d)\n",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));
	}
}

//-----------------------------------------------------------------------------
void aoap_send_string( libusb_device_handle *dev_handle, 
                      unsigned char strIndex, 
                      char *str,
                      unsigned char strLen)
{
  int r;

  r = libusb_control_transfer(dev_handle,
                              LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                              ACCESSORY_SEND_STRING,
                              0,
                              strIndex,
                              (unsigned char*)str,
                              strLen + 1,
                              1000 );
  if(r < 0)
  {
    fprintf(stderr,"Send string index %d error: %d %s\n", strIndex, r, libusb_error_name(r));
    libusb_close(dev_handle);
    libusb_exit(NULL);
    exit( EXIT_FAILURE );
  }
}

//-----------------------------------------------------------------------------
int main(void)
{
	libusb_device        **devs;
  libusb_device_handle *dev_handle;
	int                   r,
                        active_congif;
	ssize_t               cnt;
  unsigned char         buff[64];

	r = libusb_init(NULL);
	if (r < 0)
		return r;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		return (int) cnt;

	print_devs(devs);
	libusb_free_device_list(devs, 1);

  dev_handle = libusb_open_device_with_vid_pid(NULL, 0x22b8, 0x2e63);
  if(dev_handle == NULL)
  {
    fprintf(stderr,"Cannot open USB device.\n");
    goto exit;
  }

  r = libusb_get_configuration(dev_handle, &active_congif);
  if(r < 0)
  {
    fprintf(stderr,"Get active configuration error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }

  fprintf(stderr,"Active configuration: %d\n", active_congif);


  r = libusb_control_transfer(dev_handle,
                              LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                              ACCESSORY_GET_PROTOCOL,
                              0,
                              0,
                              buff,
                              sizeof(buff),
                              1000 );
  if(r < 0)
  {
    fprintf(stderr,"Get AOAP version error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }
  fprintf(stderr,"AOAP version: %d\n", buff[0]);

  aoap_send_string(dev_handle, 0, ACCESSORY_STRING_MANUFACTURER, strlen(ACCESSORY_STRING_MANUFACTURER));
  aoap_send_string(dev_handle, 1, ACCESSORY_STRING_MODEL,        strlen(ACCESSORY_STRING_MODEL));
  aoap_send_string(dev_handle, 2, ACCESSORY_STRING_DESCRIPTION,  strlen(ACCESSORY_STRING_DESCRIPTION));
  aoap_send_string(dev_handle, 3, ACCESSORY_STRING_VERSION,      strlen(ACCESSORY_STRING_VERSION));
  aoap_send_string(dev_handle, 4, ACCESSORY_STRING_URL,          strlen(ACCESSORY_STRING_URL));
  aoap_send_string(dev_handle, 5, ACCESSORY_STRING_SERIAL,       strlen(ACCESSORY_STRING_SERIAL));

  r = libusb_control_transfer(dev_handle,
                              LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                              ACCESSORY_AUDIO,
                              1,
                              0,
                              buff,
                              0,
                              1000 );
  if(r < 0)
  {
    fprintf(stderr,"AOAP audio error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }

  r = libusb_control_transfer(dev_handle,
                              LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                              ACCESSORY_START,
                              0,
                              0,
                              buff,
                              0,
                              1000 );
  if(r < 0)
  {
    fprintf(stderr,"AOAP start error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }
  libusb_close(dev_handle);
  libusb_exit(NULL);

  sleep(5);

  r = libusb_init(NULL);
  if (r < 0)
    return r;

  cnt = libusb_get_device_list(NULL, &devs);
  if (cnt < 0)
    return (int) cnt;

  print_devs(devs);
  libusb_free_device_list(devs, 1);

  dev_handle = libusb_open_device_with_vid_pid(NULL, 0x18d1, 0x2d05);
  if(dev_handle == NULL)
  {
    fprintf(stderr,"Cannot open USB device.\n");
    goto exit;
  }

  r = libusb_get_configuration(dev_handle, &active_congif);
  if(r < 0)
  {
    fprintf(stderr,"Get active configuration error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }

  fprintf(stderr,"Active configuration: %d\n", active_congif);

/*
  r = libusb_detach_kernel_driver(dev_handle, 2);
  if(r < 0)
  {
    fprintf(stderr,"Detach kernel driver error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }
*/

  r = libusb_claim_interface(dev_handle, 2);
  if(r < 0)
  {
    fprintf(stderr,"Claim interface error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }

  r = libusb_set_interface_alt_setting(dev_handle, 2, 1);
  if(r < 0)
  {
    fprintf(stderr,"Set interface alt settings error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }


  

exit:
  if(dev_handle)
    libusb_close(dev_handle);
	libusb_exit(NULL);
	return 0;
}

