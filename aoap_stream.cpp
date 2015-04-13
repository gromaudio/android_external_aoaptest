#include <cstdio>
#include <unistd.h>
#include "aoap_stream.h"

//-----------------------------------------------------------------------------
#define NUM_ISO_PACK    8
#define AOAP_VID        0x18d1
#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))

//-----------------------------------------------------------------------------
extern "C" {
const char* libusb_error_name(int errcode);
}

//--------------------------------------------------------------------------
const uint16_t aSupportedPIDs[] = { 0x2d02, 0x2d03, 0x2d04, 0x2d05 };

//--------------------------------------------------------------------------
AoapStream::AoapStream()
    : Thread(false), mStreamingActive(false)
{
}

//--------------------------------------------------------------------------
AoapStream::~AoapStream()
{
  stopStreaming();
  closeStreamingDevice();
}

//--------------------------------------------------------------------------
status_t AoapStream::readyToRun()
{
  openStreamingDevice();
  startStreaming();
  return NO_ERROR;
}

//--------------------------------------------------------------------------
void AoapStream::onFirstRef()
{
  run("AoapStream", PRIORITY_URGENT_DISPLAY);
}

//--------------------------------------------------------------------------
int AoapStream::findStreamingInterface(libusb_device_handle *dev_handle,
                                       libusb_interface_descriptor *interface_desc)
{
  int           r,
                interface_idx,
                alt_idx;
  unsigned char res;
  libusb_config_descriptor *config_desc;

  res = 0;
  r = libusb_get_active_config_descriptor(libusb_get_device(dev_handle), &config_desc);
  if(r < 0)
  {
    fprintf(stderr,"Get active configuration descriptor error: %d %s\n", r, libusb_error_name(r));
    return 0;
  }

  for(interface_idx= 0; interface_idx< config_desc->bNumInterfaces; interface_idx++)
    for(alt_idx= 0; alt_idx< config_desc->interface[interface_idx].num_altsetting; alt_idx++)
    {
      memcpy(interface_desc,
             &config_desc->interface[interface_idx].altsetting[alt_idx],
             sizeof(libusb_interface_descriptor));

      fprintf(stderr,"Int %d, Alt %d, EPs %d, Class %d, SubClass %d\n", interface_desc->bInterfaceNumber,
                                                                        interface_desc->bAlternateSetting,
                                                                        interface_desc->bNumEndpoints,
                                                                        interface_desc->bInterfaceClass,
                                                                        interface_desc->bInterfaceSubClass );
      if(interface_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
         interface_desc->bInterfaceSubClass == 2 &&
         interface_desc->bNumEndpoints != 0)
      {
        libusb_free_config_descriptor(config_desc);
        return 0;
      }
    }
  libusb_free_config_descriptor(config_desc);
  return -1;
}

//--------------------------------------------------------------------------
void AoapStream::openStreamingDevice()
{
  int r, active_congif;

  r = libusb_init(NULL);
  if(r < 0)
    return;

  for(unsigned char i = 0; i < ARRAY_SIZE(aSupportedPIDs); i++)
  {
    mDevHandle = libusb_open_device_with_vid_pid(NULL, AOAP_VID, aSupportedPIDs[ i ]);
    if(mDevHandle != NULL)
      break;
  }
  if(mDevHandle == NULL)
  {
    fprintf(stderr,"Cannot open USB device.\n");
    return;
  }

  r = libusb_get_configuration(mDevHandle, &active_congif);
  if(r < 0)
  {
    fprintf(stderr,"Get active configuration error: %d %s\n", r, libusb_error_name(r));
    goto stream_exit;
  }

  fprintf(stderr,"Active configuration: %d\n", active_congif);

  if(findStreamingInterface(mDevHandle, &mInterfaceDesc))
  {
    fprintf(stderr,"Cannot find streaming interface.\n");
    goto stream_exit;
  }
  fprintf(stderr,"Streaming interface %d, endpoint 0x%02X\n", mInterfaceDesc.bInterfaceNumber,
                                                              mInterfaceDesc.endpoint->bEndpointAddress);

  libusb_detach_kernel_driver(mDevHandle, mInterfaceDesc.bInterfaceNumber);

  r = libusb_claim_interface(mDevHandle, mInterfaceDesc.bInterfaceNumber);
  if(r < 0)
  {
    fprintf(stderr,"Claim interface error: %d %s\n", r, libusb_error_name(r));
    goto stream_exit;
  }

  r = libusb_set_interface_alt_setting(mDevHandle, mInterfaceDesc.bInterfaceNumber, mInterfaceDesc.bAlternateSetting);
  if(r < 0)
  {
    fprintf(stderr,"Set interface alt settings error: %d %s\n", r, libusb_error_name(r));
    goto stream_exit;
  }
  return;

stream_exit:
  closeStreamingDevice();
  return;
}

//--------------------------------------------------------------------------
void AoapStream::closeStreamingDevice()
{
  libusb_close(mDevHandle);
  libusb_exit(NULL);
  mDevHandle = NULL;
}

//-----------------------------------------------------------------------------
void AoapStream::iso_transfer_cb(struct libusb_transfer *xfr)
{
  int i, data_size;

  if(xfr->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
  {
    data_size = 0;
    for(i = 0; i < xfr->num_iso_packets; i++)
    {
      struct libusb_iso_packet_descriptor *pack = &xfr->iso_packet_desc[i];
      data_size += pack->actual_length;
//      printf("pack%u status:%d, length:%u, actual_length:%u\n", i, pack->status, pack->length, pack->actual_length);
    }
    fprintf(stderr,"ISO completted: %d, %d\n", (int)xfr->user_data, data_size);
  }

  libusb_submit_transfer(xfr);
//  bCompleted = 1;
}

//--------------------------------------------------------------------------
void AoapStream::startStreaming()
{
  int r, iMaxPacketSize;
  unsigned char iIsoEpNumAddr = mInterfaceDesc.endpoint->bEndpointAddress;

  iMaxPacketSize = libusb_get_max_iso_packet_size(libusb_get_device(mDevHandle), iIsoEpNumAddr);
  if(iMaxPacketSize < 0)
  {
    fprintf(stderr,"Get ISO max packet size error: %d %s\n", iMaxPacketSize, libusb_error_name(iMaxPacketSize));
    return;
  }
  fprintf(stderr,"ISO max packet size: %d\n", iMaxPacketSize);

  mIsoTransfer_1 = libusb_alloc_transfer(NUM_ISO_PACK);
  mIsoTransfer_2 = libusb_alloc_transfer(NUM_ISO_PACK);
  if(!mIsoTransfer_1 || !mIsoTransfer_2)
  {
    fprintf(stderr,"Allocating libusb_transfer error\n");
    return;
  }

  libusb_fill_iso_transfer(mIsoTransfer_1,
                           mDevHandle,
                           iIsoEpNumAddr,
                           mIsoBuffer_1,
                           sizeof(mIsoBuffer_1),
                           NUM_ISO_PACK,
                           iso_transfer_cb,
                           (void*)1,
                           0);
  libusb_fill_iso_transfer(mIsoTransfer_2,
                           mDevHandle,
                           iIsoEpNumAddr,
                           mIsoBuffer_2,
                           sizeof(mIsoBuffer_2),
                           NUM_ISO_PACK,
                           iso_transfer_cb,
                           (void*)2,
                           0);

  libusb_set_iso_packet_lengths(mIsoTransfer_1, iMaxPacketSize);
  libusb_set_iso_packet_lengths(mIsoTransfer_2, iMaxPacketSize);

  r = libusb_submit_transfer(mIsoTransfer_1);
  if(r < 0)
  {
    fprintf(stderr,"Submit transfer 1 error: %d %s\n", r, libusb_error_name(r));
    return;
  }
  r = libusb_submit_transfer(mIsoTransfer_2);
  if(r < 0)
  {
    fprintf(stderr,"Submit transfer 2 error: %d %s\n", r, libusb_error_name(r));
    return;
  }
}

//--------------------------------------------------------------------------
void AoapStream::stopStreaming()
{
  if(mIsoTransfer_1)
    libusb_free_transfer(mIsoTransfer_1);
  if(mIsoTransfer_2)
    libusb_free_transfer(mIsoTransfer_2);
}

//--------------------------------------------------------------------------
bool AoapStream::threadLoop()
{
  sp<CMessage> msg = mThreadQueue.waitMessage(1000000);

  if(msg != 0)
  {
    switch(msg->what)
    {
      case AOAP_STREAMING_START:
        fprintf(stderr, "Start streaming.\n");
        mStreamingActive = true;
        break;

      case AOAP_STREAMING_STOP:
        fprintf(stderr, "Stop streaming.\n");
        mStreamingActive = false;
        break;

      default:
        fprintf(stderr, "Unknown command: %d\n", msg->what);
        break;
    }
  }

  if(mStreamingActive)
  {
    int r = libusb_handle_events(NULL);
    if(r != LIBUSB_SUCCESS)
    {
      fprintf(stderr,"Handle event error: %d %s\n", r, libusb_error_name(r));
      libusb_cancel_transfer(mIsoTransfer_1);
      libusb_cancel_transfer(mIsoTransfer_2);
      stopStreaming();
      closeStreamingDevice();
    }
  }

  return true;
}

//--------------------------------------------------------------------------
void AoapStream::start()
{
  mThreadQueue.postMessage(new CMessage(AoapStream::AOAP_STREAMING_START));
}

//--------------------------------------------------------------------------
void AoapStream::stop()
{
  mThreadQueue.postMessage(new CMessage(AoapStream::AOAP_STREAMING_STOP));
}
