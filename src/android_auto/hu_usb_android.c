#define LOGTAG "hu_usb"
#include "hu_uti.h"    // Utilities
#include "hu_oap.h"    // Open Accessory Protocol
#include <usbhost/usbhost.h>

#define AOAP_VID 0x18d1

int iusb_state = 0; // 0: Initial    1: Startin    2: Started    3: Stoppin    4: Stopped
struct usb_host_context         *usb_context      = NULL;
struct usb_device               *usb_device       = NULL;
struct usb_descriptor_header    *desc_header;
struct usb_descriptor_iter       desc_iter;
struct usb_interface_descriptor *aoap_i_desc      = NULL;
struct usb_endpoint_descriptor  *aoap_in_ep_desc  = NULL;
struct usb_endpoint_descriptor  *aoap_out_ep_desc = NULL;

//--------------------------------------------------------------------------
int hu_usb_recv(byte * buf, int len, int tmo)
{
  int ret = -1;

  ret = usb_device_bulk_transfer(usb_device, aoap_in_ep_desc->bEndpointAddress, buf, len, tmo); // milli-second timeout
//  if(len != 65536)
//    fprintf(stderr, "USB recv %d, %d\n", len, ret);
  if(ret == -1)ret = 0;

  return ret;
}

int hu_usb_send(byte * buf, int len, int tmo)
{
  int ret = usb_device_bulk_transfer(usb_device, aoap_out_ep_desc->bEndpointAddress, buf, len, tmo);// milli-second timeout
  fprintf(stderr, "USB send: %d, %d, %d\n", ret, len, tmo);
  return ret;
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
  return 0;
}

int usb_discovery_done(void *client_data)
{
  return 0;
}

//--------------------------------------------------------------------------
int hu_usb_start(byte ep_in_addr, byte ep_out_addr)
{
  int ret = 0;

  if(iusb_state == hu_STATE_STARTED)
  {
    logd("CHECK: iusb_state: %d (%s)", iusb_state, state_get(iusb_state));
    return 0;
  }

  iusb_state = hu_STATE_STARTIN;
  logd ("  SET: iusb_state: %d (%s)", iusb_state, state_get (iusb_state));

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
    iusb_state = hu_STATE_STOPPED;
    logd ("  SET: iusb_state: %d (%s)", iusb_state, state_get (iusb_state));
    usb_device_close(usb_device);
    usb_host_cleanup(usb_context);
    return (-3);
  }
  else
  {
    iusb_state = hu_STATE_STARTED;
    logd ("  SET: iusb_state: %d (%s)", iusb_state, state_get (iusb_state));
    return (0);
  }
}

int hu_usb_stop(void)
{
  iusb_state = hu_STATE_STOPPIN;
  logd ("  SET: iusb_state: %d (%s)", iusb_state, state_get (iusb_state));
  if(usb_device != NULL)
    usb_device_close(usb_device);
  if(usb_context != NULL)
    usb_host_cleanup(usb_context);
  iusb_state = hu_STATE_STOPPED;
  logd ("  SET: iusb_state: %d (%s)", iusb_state, state_get (iusb_state));
  return 0;
}
