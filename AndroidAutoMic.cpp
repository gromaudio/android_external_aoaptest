#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <media/AudioRecord.h>
#include "AndroidAutoMic.h"

using namespace android;

//-------------------------------------------------------------------------------------------------
#define AA_CH_MIC             7
#define AAUTO_MIC_BUFF_SIZE   1024
#define AAUTO_MIC_HEAD_SIZE   14

//-------------------------------------------------------------------------------------------------
sp<AudioRecord>   gAudioRecord;
pthread_t         gAudioInputThread;
uint8_t           gAudioBuff[ AAUTO_MIC_BUFF_SIZE ];
int               gAudioDataSize;

//-------------------------------------------------------------------------------------------------
static void* audioInputThread( void* params )
{
  while( true )
  {
    while( gAudioDataSize != 0 )
      usleep( 5000000 );

    gAudioDataSize = gAudioRecord->read( gAudioBuff, AAUTO_MIC_BUFF_SIZE );
  }
}

//-------------------------------------------------------------------------------------------------
void MIC_init( void )
{
  gAudioDataSize = 0;
  gAudioRecord = new AudioRecord( AUDIO_SOURCE_DEFAULT,
                                  16000,
                                  AUDIO_FORMAT_PCM_16_BIT,
                                  AUDIO_CHANNEL_IN_MONO,
                                  0 );
  if( gAudioRecord != NULL )
  {
    gAudioRecord->start();
    fprintf( stderr, "AudioRecord OK.\n" );
  }

  if( pthread_create( &gAudioInputThread,
                      NULL,
                      audioInputThread,
                      NULL ) )
  {
    ALOGE( "Cannot create audio input thread" );
  }
}

//-------------------------------------------------------------------------------------------------
size_t MIC_getData( uint8_t *buff, size_t buff_size )
{
  size_t         bytes_to_copy;
  uint64_t       ts;
  struct timeval tv;

  if( gAudioRecord == NULL ||
      gAudioDataSize == 0  ||
      buff_size <= AAUTO_MIC_HEAD_SIZE )
  {
    return 0;
  }

  buff[ 0 ] = AA_CH_MIC;  // Mic channel
  buff[ 1 ] = 0x0b;       // Flag filled here
  buff[ 2 ] = 0x00;       // 2 bytes Length filled here
  buff[ 3 ] = 0x00;
  buff[ 4 ] = 0x00;       // Message Type = 0 for data, OR 32774 for Stop w/mandatory 0x08 int and optional 0x10 int (senderprotocol/aq -> com.google.android.e.b.ca)
  buff[ 5 ] = 0x00;

  gettimeofday( &tv, NULL );
  ts = 1000000 * tv.tv_sec + tv.tv_usec;  // ts = Timestamp (uptime) in microseconds
  for( int i = 7; i >= 0; i-- )           // Fill 8 bytes backwards
  {
    buff[ 6 + i ] = (uint8_t)( ts & 0xFF );
    ts >>= 8;
  }

  bytes_to_copy = ( gAudioDataSize <= buff_size ) ? gAudioDataSize : buff_size;
  if( bytes_to_copy > ( buff_size - AAUTO_MIC_HEAD_SIZE ) )
    bytes_to_copy -= AAUTO_MIC_HEAD_SIZE;

  memcpy( buff + AAUTO_MIC_HEAD_SIZE, gAudioBuff, bytes_to_copy );
  gAudioDataSize -= bytes_to_copy;

  return bytes_to_copy + AAUTO_MIC_HEAD_SIZE;
}
