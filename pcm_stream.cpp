#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "pcm_stream.h"

//--------------------------------------------------------------------------
void callback(int event, void* user, void *info)
{
  fprintf( stderr, "cb: %d\n", event );
}

//--------------------------------------------------------------------------
PcmStream::PcmStream( unsigned int card, unsigned int device )
    : Thread( false ), mCard( card ), mDevice( device )
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

  AudioTrack::getMinFrameCount( &frameCount,
                                AUDIO_STREAM_MUSIC,
                                44100 );

  mAudioTrack = new AudioTrack( AUDIO_STREAM_MUSIC,
                                44100,
                                AUDIO_FORMAT_PCM_16_BIT,
                                AUDIO_CHANNEL_OUT_STEREO,
                                0,
                                AUDIO_OUTPUT_FLAG_NONE,
                                callback,
                                NULL,
                                0,
                                0,
                                AudioTrack::TRANSFER_SYNC,
                                NULL,
                                -1 );

  fprintf( stderr, "AudioTrack openned. Frame count %d\n", frameCount );
  return NO_ERROR;
}

//--------------------------------------------------------------------------
void PcmStream::onFirstRef()
{
  run( "PcmStream", PRIORITY_URGENT_DISPLAY );
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

  mPcm = pcm_open( mCard, mDevice, PCM_IN, &config );
  if( !mPcm || !pcm_is_ready( mPcm ) )
  {
    fprintf( stderr, "Unable to open PCM device (%s)\n", pcm_get_error( mPcm ) );
    return;
  }

  mBufferSize = pcm_get_buffer_size( mPcm );
  mBuffer     = new char[ mBufferSize ];
  if( !mBuffer )
  {
    fprintf( stderr, "Unable to allocate %d bytes\n", mBufferSize );
    pcm_close( mPcm );
    mPcm = 0;
  }
  fprintf( stderr, "PCM card %d, dev %d, buff size %d\n", mCard, mDevice, mBufferSize );
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
    pcm_close( mPcm );
    mPcm = 0;
  }
}

//--------------------------------------------------------------------------
bool PcmStream::threadLoop()
{
  sp<CMessage> msg = mThreadQueue.waitMessage( 0 );

  if( msg != 0 )
  {
    switch( msg->what )
    {
      case PCM_STREAMING_START:
        fprintf( stderr, "Start streaming.\n" );
        openCaptureDevice();
        if(mPcm)
        {
          mStreamingActive = true;
          mAudioTrack->start();



{
  uint8_t wav_header[44] = {
  	0x52, 0x49, 0x46, 0x46, 0x24, 0x00, 0x87, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
  	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x10, 0xB1, 0x02, 0x00,
  	0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x87, 0x00
  };


  int fd = open("/boot/test.wav", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if(fd != -1)
  {
    write(fd, wav_header, sizeof(wav_header));
    close(fd);
  }
  else
  {
    fprintf( stderr, "Cannot create WAV file.\n" );
  }
}




        }
        else
        {
          fprintf( stderr, "Error. PCM device not ready.\n" );
        }
        break;

      case PCM_STREAMING_STOP:
        fprintf( stderr, "Stop streaming.\n" );
        mStreamingActive = false;
        mAudioTrack->stop();
        closeCaptureDevice();
        break;

      default:
        fprintf( stderr, "Unknown command: %d\n", msg->what );
        break;
    }
  }

  if( mPcm && mStreamingActive )
  {
    int res = pcm_read( mPcm, mBuffer, mBufferSize );
    if( !res )
    {


{
  int fd = open( "/boot/test.wav", O_WRONLY | O_APPEND );
  if( fd != -1 )
  {
    write( fd, mBuffer, mBufferSize );
    close( fd );
  }
  else
  {
    fprintf( stderr, "Cannot open WAV file.\n" );
  }
}


      ssize_t  bytes_written = mAudioTrack->write( mBuffer, mBufferSize );
      if( bytes_written != mBufferSize )
        fprintf( stderr, "Cannot write all data: %d != %d\n", bytes_written, mBufferSize );
    }
    else
    {
      fprintf( stderr, "pcm_read error: %d\n", res );
//      mThreadQueue.postMessage( new CMessage( PcmStream::PCM_STREAMING_STOP ) );
    }
  }

  return true;
}

//--------------------------------------------------------------------------
void PcmStream::start()
{
  mThreadQueue.postMessage( new CMessage( PcmStream::PCM_STREAMING_START ) );
}

//--------------------------------------------------------------------------
void PcmStream::stop()
{
  mThreadQueue.postMessage( new CMessage( PcmStream::PCM_STREAMING_STOP ) );
}
