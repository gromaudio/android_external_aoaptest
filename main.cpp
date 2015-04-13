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
#include <utils/StrongPointer.h>
#if defined(__ANDROID__) || defined(ANDROID)
  #include "libusb/libusb.h"
#else
  #include <libusb-1.0/libusb.h>
#endif
#include "aoap_stream.h"

//-----------------------------------------------------------------------------
typedef struct libusb_interface_descriptor libusb_interface_descriptor;

//-----------------------------------------------------------------------------
#define NUM_ISO_PACK                    8
#define DEFAULT_VID                     0x18d1
#define DEFAULT_PID                     0x4ee2
#define AOAP_VID                        0x18d1

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

#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))

//-----------------------------------------------------------------------------
const uint16_t aSupportedPIDs[] = { 0x2d02, 0x2d03, 0x2d04, 0x2d05 };

//-----------------------------------------------------------------------------
extern "C" {
const char* libusb_error_name(int errcode);
}

//-----------------------------------------------------------------------------
static void print_devs(libusb_device **devs)
{
  libusb_device *dev;
  int32_t i = 0;

  while((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) {
      fprintf(stderr, "failed to get device descriptor");
      return;
    }

    printf("%04x:%04x (bus %d, dev %d)\n", desc.idVendor,
                                           desc.idProduct,
                                           libusb_get_bus_number(dev),
                                           libusb_get_device_address(dev));
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
void aoap_send_string(libusb_device_handle *dev_handle,
                      uint8_t strIndex,
                      const char *str,
                      uint8_t strLen)
{
  int32_t r;

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
int activate_accessory(uint16_t VID, uint16_t PID, char bListOnly, char bReset)
{
  libusb_device_handle *dev_handle;
  int                   r,
                        exit_code,
                        active_congif;
  unsigned char         buff[64];
  unsigned char         i;

  dev_handle = 0;
  exit_code  = 0;

  r = libusb_init(NULL);
  if(r < 0)
    return -1;

  enumerate_usb_devs();

  if(bListOnly)
  {
    exit_code = -1;
    goto acc_exit;
  }

  fprintf(stderr,"Trying to open device %04x:%04x.\n", VID, PID);
  dev_handle = libusb_open_device_with_vid_pid(NULL, VID, PID);

  if(dev_handle == NULL)
  {
    for(i = 0; i < ARRAY_SIZE(aSupportedPIDs); i++)
    {
      dev_handle = libusb_open_device_with_vid_pid(NULL, AOAP_VID, aSupportedPIDs[ i ]);
      if(dev_handle != NULL)
      {
        fprintf(stderr,"Accessory mode already activated(%04x:%04x).\n", AOAP_VID, aSupportedPIDs[ i ]);
        exit_code = 0;
        goto acc_exit;
      }
    }
    fprintf(stderr,"No device found.\n");
    exit_code = -1;
    goto acc_exit;
  }

  r = libusb_get_configuration(dev_handle, &active_congif);
  if(r < 0)
  {
    fprintf(stderr,"Get active configuration error: %d %s\n", r, libusb_error_name(r));
    exit_code = -1;
    goto acc_exit;
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
    exit_code = -1;
    goto acc_exit;
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
    exit_code = -1;
    goto acc_exit;
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
    exit_code = -1;
    goto acc_exit;
  }

  if(bReset)
  {
    r = libusb_reset_device(dev_handle);
    if(r < 0)
      fprintf(stderr,"Device reset error: %d %s\n", r, libusb_error_name(r));
  }

  sleep(2);

acc_exit:
  if(dev_handle)
    libusb_close(dev_handle);
  libusb_exit(NULL);
  return exit_code;
}

//--------------------------------------------------------------------------
void start_streaming(void)
{
  int ch;
  sp<AoapStream> aoapstream = new AoapStream();

  for(;;)
  {
    ch = fgetc(stdin);

    switch(ch)
    {
      case '1':
        aoapstream->start();
        break;

      case '2':
        aoapstream->stop();
        break;

      default:
        break;
    }
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
  uint16_t  VID,
            PID;
  char      bListOnly,
            bReset,
            bStream;

  VID       = DEFAULT_VID;
  PID       = DEFAULT_PID;
  bListOnly = 0;
  bReset    = 0;
  bStream   = 0;

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


  if(activate_accessory(VID, PID, bListOnly, bReset) < 0)
  {
    if(!bListOnly)
      fprintf(stderr,"Accessory cannot be activated.\n");
    exit(EXIT_FAILURE);
  }

  if(!bStream)
    exit(EXIT_SUCCESS);

  start_streaming();

  exit(EXIT_SUCCESS);
}
