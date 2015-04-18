#include <cstdio>
#include <unistd.h>
#include "pcm_stream.h"

//--------------------------------------------------------------------------
#define PCM_CARD    2
#define PCM_DEVICE  0

//--------------------------------------------------------------------------
PcmStream::PcmStream()
    : Thread(false)
{
  mStreamingActive = false;
  mPcm = 0;
}

//--------------------------------------------------------------------------
PcmStream::~PcmStream()
{
}

//--------------------------------------------------------------------------
status_t PcmStream::readyToRun()
{
  size_t frameCount;

  AudioTrack::getMinFrameCount(&frameCount,
                                AUDIO_STREAM_MUSIC,
                                44100);

  mAudioTrack = new AudioTrack( AUDIO_STREAM_MUSIC,
                                44100,
                                AUDIO_FORMAT_PCM_16_BIT,
                                AUDIO_CHANNEL_OUT_STEREO,
                                0);
  return NO_ERROR;
}

//--------------------------------------------------------------------------
void PcmStream::onFirstRef()
{
  run("PcmStream", PRIORITY_URGENT_DISPLAY);
}

//--------------------------------------------------------------------------
void PcmStream::openCaptureDevice()
{
  pcm_config config;

  config.channels          = 2;
  config.rate              = 44100;
  config.period_size       = 256;
  config.period_count      = 2;
  config.format            = PCM_FORMAT_S16_LE;
  config.start_threshold   = 0;
  config.stop_threshold    = 0;
  config.silence_threshold = 0;

  mPcm = pcm_open(PCM_CARD, PCM_DEVICE, PCM_IN, &config);
  if(!mPcm || !pcm_is_ready(mPcm))
  {
    fprintf(stderr, "Unable to open PCM device (%s)\n",
            pcm_get_error(mPcm));
    return;
  }

  mBufferSize = pcm_get_buffer_size(mPcm);
  mBuffer     = new char[mBufferSize];
  if(!mBuffer)
  {
    fprintf(stderr, "Unable to allocate %d bytes\n", mBufferSize);
    pcm_close(mPcm);
    mPcm = 0;
  }
}

//--------------------------------------------------------------------------
void PcmStream::closeCaptureDevice()
{
  if(mBuffer)
  {
    delete [] mBuffer;
  }

  if(mPcm)
  {
    pcm_close(mPcm);
    mPcm = 0;
  }
}

//--------------------------------------------------------------------------
bool PcmStream::threadLoop()
{
  sp<CMessage> msg = mThreadQueue.waitMessage(0);

  if(msg != 0)
  {
    switch(msg->what)
    {
      case PCM_STREAMING_START:
        fprintf(stderr, "Start streaming.\n");
        openCaptureDevice();
        if(mPcm)
        {
          mStreamingActive = true;
          mAudioTrack->start();
        }
        else
        {
          fprintf(stderr, "Error. PCM device not ready.\n");
        }
        break;

      case PCM_STREAMING_STOP:
        fprintf(stderr, "Stop streaming.\n");
        mStreamingActive = false;
        mAudioTrack->stop();
        closeCaptureDevice();
        break;

      default:
        fprintf(stderr, "Unknown command: %d\n", msg->what);
        break;
    }
  }

  if(mPcm && mStreamingActive)
  {
    if(!pcm_read(mPcm, mBuffer, mBufferSize))
    {
      mAudioTrack->write(mBuffer, mBufferSize);
    }
    else
    {
      mThreadQueue.postMessage(new CMessage(PcmStream::PCM_STREAMING_STOP));
    }
  }

  return true;
}

//--------------------------------------------------------------------------
void PcmStream::start()
{
  mThreadQueue.postMessage(new CMessage(PcmStream::PCM_STREAMING_START));
}

//--------------------------------------------------------------------------
void PcmStream::stop()
{
  mThreadQueue.postMessage(new CMessage(PcmStream::PCM_STREAMING_STOP));
}
