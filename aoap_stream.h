#ifndef __AOAP_STREAM_H__
#define __AOAP_STREAM_H__

#include <utils/threads.h>
#include "messageQueue.h"
#if defined(__ANDROID__) || defined(ANDROID)
  #include "libusb/libusb.h"
#else
  #include <libusb-1.0/libusb.h>
#endif


using namespace android;

class AoapStream : public Thread
{
public:
  AoapStream();
  ~AoapStream();

  void start();
  void stop();

  enum AoapStreamCommands
  {
    AOAP_STREAMING_START,
    AOAP_STREAMING_STOP
  };

private:
  virtual status_t  readyToRun();
  virtual void      onFirstRef();
  virtual bool      threadLoop();

  int  findStreamingInterface(libusb_device_handle *dev_handle,
                              libusb_interface_descriptor *interface_desc);
  void openStreamingDevice();
  void closeStreamingDevice();
  void startStreaming();
  void stopStreaming();

  static void iso_transfer_cb(struct libusb_transfer *xfr);


  CMessageQueue                mThreadQueue;
  libusb_device_handle        *mDevHandle;
  libusb_interface_descriptor  mInterfaceDesc;
  libusb_transfer             *mIsoTransfer_1,
                              *mIsoTransfer_2;
  unsigned char                mIsoBuffer_1[4096];
  unsigned char                mIsoBuffer_2[4096];
  bool                         mStreamingActive;
};


#endif /* __AOAP_STREAM_H_ */
