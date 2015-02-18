/*
* Android accessory protocol tester.
*
* This program can be used and distributed without restrictions.
*
* Authors: Ivan Zaitsev
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#if defined(__ANDROID__) || defined(ANDROID)
  #include "libusb/libusb.h"
#else
  #include <libusb-1.0/libusb.h>
#endif

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
const char* libusb_error_name(int errcode);

//-----------------------------------------------------------------------------
void iso_transfer_cb(struct libusb_transfer *transfer)
{
  fprintf(stderr,"ISO completted: %d\n", transfer->status);
}

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
static void enumerate_usb_devs(void)
{
  libusb_device **devs;

  libusb_get_device_list(NULL, &devs);
  print_devs(devs);
  libusb_free_device_list(devs, 1);
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

//--------------------------------------------------------------------------
static void usage(FILE * fp, int argc, char ** argv)
{
  fprintf( fp,
           "Usage: %s [options]\n\n"
           "Options:\n"
           "-v | --vid Device VID [18d1]\n"
           "-p | --pid Device PID [4ee2]\n"
           "-h | --help Print this message\n"
           "-l | --list List available devices\n"
           "-r | --reset Perform port reset after activating accessory\n"
           "-s | --stream Start audio streaming after activating accessory\n"
           "",
           argv[0] );
}

//--------------------------------------------------------------------------
static const char short_options[] = "v:p:hlrs";
static const struct option long_options[] =
{
  { "vid",  required_argument, NULL, 'v' },
  { "pid",  required_argument, NULL, 'p' },
  { "help",       no_argument, NULL, 'h' },
  { "list",       no_argument, NULL, 'l' },
  { "reset",      no_argument, NULL, 'r' },
  { "stream",     no_argument, NULL, 's' },
  { 0, 0, 0, 0 }
};

//-----------------------------------------------------------------------------
int main(int argc, char ** argv)
{
  libusb_device_handle *dev_handle;
	int                   r,
                        active_congif;
	ssize_t               cnt;
  uint16_t              VID,
                        PID;
  char                  bListOnly,
                        bReset,
                        bStream;
  unsigned char         buff[64];

  VID = 0x18d1;
  PID = 0x4ee2;
  bListOnly = 0;
  bReset    = 0;
  bStream   = 0;
  dev_handle= 0;

  for(;;)
  {
    int index;
    int c;

    c = getopt_long(argc, argv, short_options, long_options, &index);
    if(-1 == c)
      break;

    switch(c)
    {
      case 0:
        break;

      case 'v':
        VID = strtol(optarg, NULL, 16);
        break;

      case 'p':
        PID = strtol(optarg, NULL, 16);
        break;

      case 'h':
        usage(stdout, argc, argv);
        exit( EXIT_SUCCESS );
        break;

      case 'l':
        bListOnly = 1;
        break;

      case 'r':
        bReset = 1;
        break;

      case 's':
        bStream = 1;
        break;

      default:
        usage(stderr, argc, argv);
        exit(EXIT_FAILURE);
        break;
    }
  }

	r = libusb_init(NULL);
	if (r < 0)
		return r;

	enumerate_usb_devs();

  if(bListOnly)
    goto exit;

  fprintf(stderr,"Trying to open device %04x:%04x.\n", VID, PID);
  dev_handle = libusb_open_device_with_vid_pid(NULL, VID, PID);
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

  if(bReset)
  {
    r = libusb_reset_device(dev_handle);
    if(r < 0)
      fprintf(stderr,"Device reset error: %d %s\n", r, libusb_error_name(r));
  }

  libusb_close(dev_handle);
  libusb_exit(NULL);

  if(!bStream)
    exit( EXIT_SUCCESS );

  sleep(3);

  r = libusb_init(NULL);
  if (r < 0)
    return r;

  enumerate_usb_devs();

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

  r = libusb_detach_kernel_driver(dev_handle, 2);
  if(r < 0)
  {
    fprintf(stderr,"Detach kernel driver error: %d %s\n", r, libusb_error_name(r));
    goto exit;
  }

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


{
struct libusb_transfer *iso_transfer;
unsigned char iso_buffer[1024];
int iMaxPacketSize;


iMaxPacketSize = libusb_get_max_iso_packet_size(libusb_get_device(dev_handle), 0x82);
if(iMaxPacketSize < 0)
{
  fprintf(stderr,"Get ISO max packet size error: %d %s\n", iMaxPacketSize, libusb_error_name(iMaxPacketSize));
  goto exit;
}
fprintf(stderr,"ISO max packet size: %d\n", iMaxPacketSize);

iso_transfer = libusb_alloc_transfer(8);
if(iso_transfer == 0)
{
  fprintf(stderr,"Allocating libusb_transfer error\n");
  goto exit;
}

libusb_set_iso_packet_lengths(iso_transfer, iMaxPacketSize);

libusb_fill_iso_transfer(iso_transfer,
                         dev_handle,
                         0x82,
                         iso_buffer,
                         sizeof(iso_buffer),
                         4,
                         iso_transfer_cb,
                         0,
                         1000);

r = libusb_submit_transfer(iso_transfer);
if(r < 0)
{
  fprintf(stderr,"Submit transfer error: %d %s\n", r, libusb_error_name(r));
  goto exit;
}

sleep(2);

libusb_free_transfer(iso_transfer);
}


exit:
fprintf(stderr,"111\n");
  if(dev_handle)
    libusb_close(dev_handle);
fprintf(stderr,"222\n");
	libusb_exit(NULL);
fprintf(stderr,"333\n");
	return 0;
}

