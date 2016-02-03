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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utils/StrongPointer.h>
#include <utils/Timers.h>
#include <usbhost/usbhost.h>
#include "libusb/libusb.h"
#include "AndroidAuto.h"
#include "pcm_stream.h"

extern "C" {
#include "common.h"
#include "intercom/usbhost_bulk.h"
#include "interface/browser_interface.h"
#include "interface/android/android_browser.h"
#include "interface/android/android_commands.h"
}

using namespace android;

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

#define AALINQ_STRING_MANUFACTURER      "Gromaudio"
#define AALINQ_STRING_MODEL             "GROMLinQ"
#define AALINQ_STRING_DESCRIPTION       "Accessory Linq"
#define AALINQ_STRING_VERSION           "0.9"
#define AALINQ_STRING_URL               "http://www.gromaudio.com/aalinq"
#define AALINQ_STRING_SERIAL            "0000000012345678"

#define AUTO_STRING_MANUFACTURER        "Android"
#define AUTO_STRING_MODEL               "Android Auto"
#define AUTO_STRING_DESCRIPTION         "Head Unit"
#define AUTO_STRING_VERSION             "1.0"
#define AUTO_STRING_URL                 "http://www.android.com/"
#define AUTO_STRING_SERIAL              "0"

#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))

//-----------------------------------------------------------------------------
const uint16_t aSupportedPIDs[] = { 0x2d02, 0x2d03, 0x2d04, 0x2d05 };
bool gQuit;

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
int activate_accessory(uint16_t VID, uint16_t PID, bool bListOnly, bool bReset, bool bAuto)
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

  if(!bAuto)
  {
    aoap_send_string(dev_handle, 0, AALINQ_STRING_MANUFACTURER, strlen(AALINQ_STRING_MANUFACTURER));
    aoap_send_string(dev_handle, 1, AALINQ_STRING_MODEL,        strlen(AALINQ_STRING_MODEL));
    aoap_send_string(dev_handle, 2, AALINQ_STRING_DESCRIPTION,  strlen(AALINQ_STRING_DESCRIPTION));
    aoap_send_string(dev_handle, 3, AALINQ_STRING_VERSION,      strlen(AALINQ_STRING_VERSION));
    aoap_send_string(dev_handle, 4, AALINQ_STRING_URL,          strlen(AALINQ_STRING_URL));
    aoap_send_string(dev_handle, 5, AALINQ_STRING_SERIAL,       strlen(AALINQ_STRING_SERIAL));
  }
  else
  {
    aoap_send_string(dev_handle, 0, AUTO_STRING_MANUFACTURER, strlen(AUTO_STRING_MANUFACTURER));
    aoap_send_string(dev_handle, 1, AUTO_STRING_MODEL,        strlen(AUTO_STRING_MODEL));
    aoap_send_string(dev_handle, 2, AUTO_STRING_DESCRIPTION,  strlen(AUTO_STRING_DESCRIPTION));
    aoap_send_string(dev_handle, 3, AUTO_STRING_VERSION,      strlen(AUTO_STRING_VERSION));
    aoap_send_string(dev_handle, 4, AUTO_STRING_URL,          strlen(AUTO_STRING_URL));
    aoap_send_string(dev_handle, 5, AUTO_STRING_SERIAL,       strlen(AUTO_STRING_SERIAL));
  }

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
static void got_signal( int )
{
  gQuit = true;
}

//--------------------------------------------------------------------------
void start_streaming(void)
{
  int                              nodesCount,
                                   itemsCount;
  char                             itemName[100];
  char                             inputBuffer[20];
  char                             *input;
  DEVICE_BROWSER_ITEM              item;
  const DEVICE_BROWSER_INTERFACE  *pAndroidBrowser;
  ANDROID_BROWSER_STATE            AndroidBrowserState;
  ANDROID_COMMAND_STATE            AndroidCommandState;
  sp<PcmStream>                    pcm_stream = new PcmStream( 2, 0 );
  struct sigaction                 sa;

  gQuit = false;
  memset( &sa, 0, sizeof( sa ) );
  sa.sa_handler = got_signal;
  sigfillset( &sa.sa_mask );
  sigaction( SIGINT, &sa, NULL );

  pcm_stream->setSampleRate( 44100 );
  ACOMMAND_Init();
  ABROWSER_Init();
  if( OK == Bulk_DriveInsert())
  {
    ACOMMAND_Activate(&AndroidCommandState);
    ABROWSER_Activate(&AndroidBrowserState);
    pAndroidBrowser = ABROWSER_GetInterface();
    item.name       = itemName;
    item.nameSize   = sizeof(itemName);

    fprintf(stderr, "Browser commands:\n");
    fprintf(stderr, "  r        - scan root.\n");
    fprintf(stderr, "  g id     - scan group by its Id.\n");
    fprintf(stderr, "  g id idx - scan group by its Id and item index.\n");

    while( !gQuit )
    {
      usleep(10000);

      input = fgets(inputBuffer, sizeof(inputBuffer), stdin);
      if(input != NULL)
      {
        switch(input[0])
        {
          case '1':
            pcm_stream->start();
            break;

          case '2':
            pcm_stream->stop();
            break;

          case 'r':
            fprintf(stderr, "Scan root:\n");

            pAndroidBrowser->setNode(BROWSER_NODE_ROOT);
            pAndroidBrowser->getCount(&nodesCount, &itemsCount);
            fprintf(stderr, "Nodes %d, Items %d\n", nodesCount, itemsCount);

            for(int i = 0; i < nodesCount; i++)
            {
              pAndroidBrowser->getNode(i, &item);
              fprintf(stderr, "  %s\n", item.name);
            }
            break;

          case 'g':
            if(strlen(input) >= 3)
            {
              fprintf(stderr, "Scan group %d:\n", input[2] - 0x30);
              pAndroidBrowser->setNode(BROWSER_NODE_ROOT);
              pAndroidBrowser->getCount(&nodesCount, &itemsCount);
              pAndroidBrowser->setNode(input[2] - 0x30);
              pAndroidBrowser->getCount(&nodesCount, &itemsCount);
              fprintf(stderr, "Nodes %d, Items %d\n", nodesCount, itemsCount);
              for(int i = 0; i < nodesCount; i++)
              {
                pAndroidBrowser->getNode(i, &item);
                fprintf(stderr, "  %s\n", item.name);
              }
            }

            if(strlen(input) >= 5)
            {
              pAndroidBrowser->setNode(input[4] - 0x30);
              pAndroidBrowser->getCount(&nodesCount, &itemsCount);
              fprintf(stderr, "Nodes %d, Items %d\n", nodesCount, itemsCount);
              for(int i = 0; i < itemsCount; i++)
              {
                pAndroidBrowser->getItem(i, &item);
                fprintf(stderr, "  %s\n", item.name);
              }
            }
            break;

          default:
            break;
        }
      }
    }
    pcm_stream->stop();
    usleep( 10000 );
  }
}

//--------------------------------------------------------------------------
void start_auto( void )
{
  struct sigaction sa;

  gQuit = false;
  memset( &sa, 0, sizeof( sa ) );
  sa.sa_handler = got_signal;
  sigfillset( &sa.sa_mask );
  sigaction( SIGINT, &sa, NULL );

  if( OK != AUTO_init() )
  {
    fprintf(stderr, "22\n" );
    return;
  }

  while( !gQuit )
  {
    if( OK != AUTO_tick() )
      break;
  }
  AUTO_exit();
  usleep( 10000 );
}

//--------------------------------------------------------------------------
int usb_device_added(const char *dev_name, void *client_data)
{
  struct usb_device **usb_device = (struct usb_device**)client_data;

  *usb_device = usb_device_open(dev_name);
  if(*usb_device != NULL)
  {
    if( AOAP_VID == usb_device_get_vendor_id(*usb_device))
      return 1;

    usb_device_close(*usb_device);
  }
  return 0;
}

int usb_device_removed(const char *dev_name, void *client_data)
{
  fprintf(stderr, "usb removed cb: %s\n", dev_name);
  return 0;
}

int usb_discovery_done(void *client_data)
{
  fprintf(stderr, "usb discovery cb\n");
  return 0;
}

//--------------------------------------------------------------------------
static void test_usb(void)
{
  int err;
  struct usb_host_context         *usb_context;
  struct usb_device               *usb_device;
  struct usb_descriptor_header    *desc_header;
  struct usb_descriptor_iter       desc_iter;
  struct usb_interface_descriptor *aoap_i_desc      = NULL;
  struct usb_endpoint_descriptor  *aoap_in_ep_desc  = NULL;
  struct usb_endpoint_descriptor  *aoap_out_ep_desc = NULL;

  usb_context = usb_host_init();
  usb_host_run(usb_context,
               usb_device_added,
               usb_device_removed,
               usb_discovery_done,
               &usb_device);
  fprintf(stderr, "AOAP device found: %04x, %s\n", usb_device_get_vendor_id(usb_device),
                                                   usb_device_get_name(usb_device));

  fprintf(stderr, "    manufacturer:  %s\n", usb_device_get_manufacturer_name(usb_device));
  fprintf(stderr, "    product:       %s\n", usb_device_get_product_name(usb_device));

  usb_descriptor_iter_init(usb_device, &desc_iter);
  while((desc_header = usb_descriptor_iter_next(&desc_iter)) != NULL)
  {
    switch(desc_header->bDescriptorType)
    {
      case USB_DT_INTERFACE:
      {
        struct usb_interface_descriptor *desc = (struct usb_interface_descriptor *)desc_header;

        if(aoap_i_desc == NULL)
        {
          fprintf(stderr, "INTERFACE DESCRIPTOR:\n");
          fprintf(stderr, "    length:    %d\n",      desc->bLength);
          fprintf(stderr, "    type:      0x%02X\n",  desc->bDescriptorType);
          fprintf(stderr, "    number:    %d\n",      desc->bInterfaceNumber);
          fprintf(stderr, "    altset:    %d\n",      desc->bAlternateSetting);
          fprintf(stderr, "    endpoints: %d\n",      desc->bNumEndpoints);
          fprintf(stderr, "    class:     0x%02X\n",  desc->bInterfaceClass);
          fprintf(stderr, "    subclass:  0x%02X\n",  desc->bInterfaceSubClass);
          fprintf(stderr, "    protocol:  0x%02X\n",  desc->bInterfaceProtocol);
          fprintf(stderr, "    interface: %d\n",      desc->iInterface);

          if( desc->bNumEndpoints == 2 &&
              desc->bInterfaceClass == USB_CLASS_VENDOR_SPEC &&
              desc->bInterfaceSubClass == USB_SUBCLASS_VENDOR_SPEC )
          {
            aoap_i_desc = desc;
          }
        }
        break;
      }

      case USB_DT_ENDPOINT:
      {
        struct usb_endpoint_descriptor *desc = (struct usb_endpoint_descriptor *)desc_header;

        if(aoap_in_ep_desc == NULL ||
           aoap_out_ep_desc == NULL)
        {
          fprintf(stderr, "    ENDPOINT DESCRIPTOR:\n");
          fprintf(stderr, "        length:     %d\n",      desc->bLength);
          fprintf(stderr, "        type:       0x%02X\n",  desc->bDescriptorType);
          fprintf(stderr, "        address:    0x%02X\n",  desc->bEndpointAddress);
          fprintf(stderr, "        attributes: %d\n",      desc->bmAttributes);
          fprintf(stderr, "        size:       %d\n",      desc->wMaxPacketSize);
          fprintf(stderr, "        interval:   %d\n",      desc->bInterval);

          if(aoap_i_desc != NULL)
          {
            if(desc->bEndpointAddress & USB_DIR_IN)
              aoap_in_ep_desc  = desc;
            else
              aoap_out_ep_desc = desc;
          }
        }
        break;
      }
    }
  }

  fprintf(stderr, "    interface:     %d\n",     aoap_i_desc->bInterfaceNumber);
  fprintf(stderr, "    endpoint IN:   0x%02X\n", aoap_in_ep_desc->bEndpointAddress);
  fprintf(stderr, "    endpoint OUT:  0x%02X\n", aoap_out_ep_desc->bEndpointAddress);

  if(usb_device_claim_interface(usb_device, aoap_i_desc->bInterfaceNumber))
  {
    fprintf(stderr, "interface claim error: %d, %s\n", errno, strerror(errno));
  }
  else
  {
    fprintf(stderr, "interface claim OK\n");
  }
}

//--------------------------------------------------------------------------
static void usage(FILE * fp, int argc, char ** argv)
{
  fprintf( fp,
           "Usage: %s [options]\n\n"
           "Options:\n"
           "-v | --vid    Device VID [18d1]\n"
           "-p | --pid    Device PID [4ee2]\n"
           "-h | --help   Print this message\n"
           "-l | --list   List available devices\n"
           "-r | --reset  Perform port reset after activating accessory\n"
           "-s | --stream Start audio streaming after activating accessory\n"
           "-a | --auto   Activate Android auto\n"
           "",
           argv[0] );
}

//--------------------------------------------------------------------------
static const char short_options[] = "v:p:hlrsa";
static const struct option long_options[] =
{
  { "vid",  required_argument, NULL, 'v' },
  { "pid",  required_argument, NULL, 'p' },
  { "help",       no_argument, NULL, 'h' },
  { "list",       no_argument, NULL, 'l' },
  { "reset",      no_argument, NULL, 'r' },
  { "stream",     no_argument, NULL, 's' },
  { "auto",       no_argument, NULL, 'a' },
  { 0, 0, 0, 0 }
};

//-----------------------------------------------------------------------------
int main(int argc, char ** argv)
{
  uint16_t  VID,
            PID;
  bool      bListOnly,
            bReset,
            bStream,
            bAuto;

  VID       = DEFAULT_VID;
  PID       = DEFAULT_PID;
  bListOnly = false;
  bReset    = false;
  bStream   = false;
  bAuto     = false;

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
        bListOnly = true;
        break;

      case 'r':
        bReset = true;
        break;

      case 's':
        bStream = true;
        break;

      case 'a':
        bAuto = true;
        break;

      default:
        usage(stderr, argc, argv);
        exit(EXIT_FAILURE);
        break;
    }
  }


  if(activate_accessory(VID, PID, bListOnly, bReset, bAuto) < 0)
  {
    if(!bListOnly)
      fprintf(stderr,"Accessory cannot be activated.\n");
    exit(EXIT_FAILURE);
  }

  if(bAuto)
    start_auto();

  if(bStream)
    start_streaming();

  exit(EXIT_SUCCESS);
}
