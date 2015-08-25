#ifndef __PCM_STREAM_H__
#define __PCM_STREAM_H__

#include <utils/threads.h>
#include <media/AudioTrack.h>
#include <tinyalsa/asoundlib.h>
#include "messageQueue.h"


using namespace android;

class PcmStream : public Thread
{
public:
  PcmStream( unsigned int card = 0, unsigned int device = 0 );
  ~PcmStream();

  void start();
  void stop();

  enum PcmStreamCommands
  {
    PCM_STREAMING_START,
    PCM_STREAMING_STOP
  };

private:
  virtual status_t  readyToRun();
  virtual void      onFirstRef();
  virtual bool      threadLoop();

  void openCaptureDevice();
  void closeCaptureDevice();

  CMessageQueue                mThreadQueue;
  pcm                         *mPcm;
  char                        *mBuffer;
  uint32_t                     mBufferSize;
  bool                         mStreamingActive;
  unsigned int                 mCard;
  unsigned int                 mDevice;
  sp<AudioTrack>               mAudioTrack;
};


#endif /* __PCM_STREAM_H_ */
