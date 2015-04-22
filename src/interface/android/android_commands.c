/*****************************************************************************
 *   android_data_wrapper.c: Wrapper over Android data interface.
 *                           Handles mutual access to android data channel.
 *
 *   Copyright(C) 2013, X-mediatech
 *   All rights reserved.
 *
 *
 *    No. | Date        |   Author       | Description
 *   ========================================================================
 *      1 | 29 Jan 2014 |  Ivan Zaitsev  | First release.
 *      2 | 13 Aug 2014 |  Ivan Zaitsev  | Timeout used for mutex access.
 *
******************************************************************************/

#include <string.h>
#include "common.h"
#if !defined(__ANDROID__) && !defined(ANDROID)
  #include "intercom/usbhost.h"
  #include "interface/debug/debug.h"
  #include "intercom/usbhost_adb.h"
  #include "intercom/usbhost.h"
  #include "intercom/usbhost_iso.h"
#endif
#include "interface/debug/debug.h"
#include "intercom/usbhost_bulk.h"
#include "interface/android/android_accessory.h"
#include "interface/android/android_commands.h"

// --------------------------------------------------------------------------------
#if !defined(__ANDROID__)
  #define ANDROID_OUT( ... )      (( HostGetUsbInterface() == USB_INTERFACE_ADB ) ? ADB_OUT( __VA_ARGS__ ) : Bulk_OUT( __VA_ARGS__ ))
  #define ANDROID_IN( ... )       (( HostGetUsbInterface() == USB_INTERFACE_ADB ) ? ADB_IN( __VA_ARGS__ ) : Bulk_IN( __VA_ARGS__ ))
#else
  #define ANDROID_OUT( ... )      Bulk_OUT( __VA_ARGS__ )
  #define ANDROID_IN( ... )       Bulk_IN( __VA_ARGS__ )
  #define HostGetAudioStreaming() (true)
#endif

#define ACOMMAND_MUTEX_TIMEOUT  ( 1000/OS_TICK_RATE_MS )

// --------------------------------------------------------------------------------
const unsigned char aMsgSet[]= {
//Msg type                    Length      Msg code        Arguments
  MESSAGE_TYPE_CONTROL,       0x00, 0x01, CONTROL_PLAY,
  MESSAGE_TYPE_CONTROL,       0x00, 0x01, CONTROL_PAUSE,
  MESSAGE_TYPE_CONTROL,       0x00, 0x01, CONTROL_STOP,
  MESSAGE_TYPE_CONTROL,       0x00, 0x01, CONTROL_FFW,
  MESSAGE_TYPE_CONTROL,       0x00, 0x01, CONTROL_FRW,
  MESSAGE_TYPE_CONTROL,       0x00, 0x01, CONTROL_NEXT_TRK,
  MESSAGE_TYPE_CONTROL,       0x00, 0x01, CONTROL_PREV_TRK,
  MESSAGE_TYPE_CONTROL,       0x00, 0x02, CONTROL_SET_RND, 0x00,
  MESSAGE_TYPE_CONTROL,       0x00, 0x02, CONTROL_SET_RPT, 0x00,
  MESSAGE_TYPE_CONTROL,       0x00, 0x02, CONTROL_RND, 0x00,
  MESSAGE_TYPE_CONTROL,       0x00, 0x02, CONTROL_RPT, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x01, MEDIA_GET_CUR_TRK,
  MESSAGE_TYPE_MEDIA,         0x00, 0x03, MEDIA_SET_CUR_TRK, 0x00, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x03, MEDIA_GET_TRK_INFO, 0x00, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x03, MEDIA_GET_TRK_ID3_INFO, 0x00, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x06, MEDIA_GET_GROUP_ITEMS, GROUP_PLAYLISTS, 0x00, 0x00, 0x00, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x02, MEDIA_GET_GROUP_ITEMS_COUNT, GROUP_PLAYLISTS,
  MESSAGE_TYPE_MEDIA,         0x00, 0x01, MEDIA_GET_TRK_COUNT,
  MESSAGE_TYPE_MEDIA,         0x00, 0x05, MEDIA_SET_TRK_TIME, 0x00, 0x00, 0x00, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x01, MEDIA_GET_STATUS,
  MESSAGE_TYPE_MEDIA,         0x00, 0x08, MEDIA_SET_STATUS, STATUS_STOP, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x04, MEDIA_SET_ACTIVE_GROUP_ITEM, GROUP_PLAYLISTS, 0x00, 0x00,
  MESSAGE_TYPE_MEDIA,         0x00, 0x05, MEDIA_GET_TRACK_NAME, 0x00, 0x00, 0x00, 0x00,
  MESSAGE_TYPE_CAPABILITIES,  0x00, 0x01, CAP_GET_DEVICE_NAME,
  MESSAGE_TYPE_CAPABILITIES,  0x00, 0x02, CAP_SET_DIGITAL_AUDIO_MODE, DIGITAL_AUDIO_AOA_V2,
  MESSAGE_TYPE_BULK,          0x00, 0x09, BULK_READ_STREAM_DATA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00
};

static const unsigned char wav_header_android[44]={
                      'R','I','F','F',      //  0 - ChunkID
                      0xF7,0xFF,0xFF,0xFF,  //  4 - ChunkSize (filesize-8)
                      'W','A','V','E',      //  8 - Format
                      'f','m','t',' ',      // 12 - SubChunkID
                      16,0,0,0,             // 16 - SubChunk1ID  // 16 for PCM
                      1,0,                  // 20 - AudioFormat (1=Uncompressed)
                      2,0,                  // 22 - NumChannels
                      0x44,0xAC,0,0,        // 24 - SampleRate in Hz
                      0x10,0xB1,2,0,        // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
                      4,0,                  // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
                      16,0,                 // 34 - BitsPerSample
                      'd','a','t','a',      // 36 - Subchunk2ID
                      0xD3,0xFF,0xFF,0xFF   // 40 - Subchunk2Size
                      };

// --------------------------------------------------------------------------------
ANDROID_COMMAND_STATE *pAndroidCommanState;
OS_MUTEX_TYPE          CmdMutex;

//-----------------------------------------------------------------------
// Find CMD and copy it into output buffer.
static void prepareCmd( unsigned char iType,
                        unsigned char iCmd,
                        volatile unsigned char* aOutBuf,
                        unsigned int iBuffSize )
{
  const unsigned char *pMsg = aMsgSet;

  // Find the message
  while( ( pMsg[ 0 ] != 0 ) && (( pMsg[ 3 ] != iCmd ) || ( pMsg[ 0 ] != iType )) )
    pMsg+= 3 + ReadBE16U( ( volatile  unsigned char *)( pMsg + 1 ));

  // Clear the message buffer and fillit in with the message found.
  memset( (void*)aOutBuf, sizeof( aOutBuf ), 0 );
  if( pMsg[ 0 ] != 0 )
    memcpy( (void*)aOutBuf, pMsg, 3 + ReadBE16U( ( volatile  unsigned char *)( pMsg + 1 ) ) );

  // Assert if no template for message defined.
  configASSERT( pMsg[ 0 ] != 0 );
}

// --------------------------------------------------------------------------------
// Init Android Command subsystem.
void ACOMMAND_Init( void )
{
  if( !OS_CreateMutex( &CmdMutex ) )
    configASSERT(false);
}

// --------------------------------------------------------------------------------
// Activate Android Command subsystem.
void ACOMMAND_Activate( ANDROID_COMMAND_STATE *pState )
{
  unsigned int iSize = 0;

  pAndroidCommanState                     = pState;
  pAndroidCommanState->iBuffSize          = ANDROID_BUFF_SIZE;
  pAndroidCommanState->bIsoAudioStreaming = FALSE;

  // select audio data transfer mode.
  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_CAPABILITIES,
                CAP_SET_DIGITAL_AUDIO_MODE,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    pAndroidCommanState->pBuff[ 4 ] = DIGITAL_AUDIO_AOA_V2;

    if( HostGetAudioStreaming() &&
        ( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) ) )
      if( OK == ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize ) )
        if( ( pAndroidCommanState->pBuff[ 3 ] == CAP_ACK ) &&
            ( pAndroidCommanState->pBuff[ 4 ] == CAP_SET_DIGITAL_AUDIO_MODE ) &&
            ( pAndroidCommanState->pBuff[ 5 ] == RESULT_OK ) )
          pAndroidCommanState->bIsoAudioStreaming = TRUE;

    OS_UnlockMutex( &CmdMutex );
  }
}

// --------------------------------------------------------------------------------
// Send status information
void ACOMMAND_SendStatus( unsigned int iTime,
                          unsigned char iPlaybackStatus,
                          unsigned short iPlaybackFlags )
{
  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_SET_STATUS,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );
    WriteBE32U( pAndroidCommanState->pBuff + 5, 1000 * iTime );
    WriteBE16U( pAndroidCommanState->pBuff + 9, iPlaybackFlags );
    pAndroidCommanState->pBuff[ 4 ] = iPlaybackStatus;
    ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );
    OS_UnlockMutex( &CmdMutex );
  }
}

// --------------------------------------------------------------------------------
// Seek command
void ACOMMAND_Seek( unsigned int iTimeMS )
{
  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_SET_TRK_TIME,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );
    WriteBE32U( pAndroidCommanState->pBuff + 4, iTimeMS );
    ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );
    OS_UnlockMutex( &CmdMutex );
  }
}

// --------------------------------------------------------------------------------
// Read status information from android device
unsigned int ACOMMAND_ReadStatus( unsigned char *pBuff,
                                  unsigned int iBuffSize )
{
  unsigned int iSize = 0;

  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_GET_STATUS,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );
    ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );
    ANDROID_IN( pBuff, iBuffSize, &iSize );
    OS_UnlockMutex( &CmdMutex );
  }
  return iSize;
}

// --------------------------------------------------------------------------------
// Get device name using capabilities command.
void ACOMMAND_GetDeviceName( char *pName, unsigned char iNameSize )
{
  unsigned int iSize;

  // No need to do anything if name buffer isn't valid.
  if(    pName == NULL
      || iNameSize == 0
      || pAndroidCommanState->bIsoAudioStreaming == FALSE )
      // We only use device name when streaming via USB
    return;

  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_CAPABILITIES,
                CAP_GET_DEVICE_NAME,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    if( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) )
    {
      if( OK == ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize ) )
      {
        unsigned char iLen =  ( pAndroidCommanState->pBuff[ 4 ] > ( iNameSize - 1 ) ) ?
                              ( iNameSize - 1 ) :
                              pAndroidCommanState->pBuff[ 4 ];
        memcpy( (void*)pName, (void*)( pAndroidCommanState->pBuff + 5 ), iLen );
        pName[ iLen ] = 0;
      }
    }
    OS_UnlockMutex( &CmdMutex );
  }
  else
  {
    pName[ 0 ] = 0;
  }
}

// --------------------------------------------------------------------------------
// Change current track in the active group
void ACOMMAND_SelectCurrentTrack( int iTrackNum )
{
  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_SET_CUR_TRK,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );
    WriteBE16U( ( pAndroidCommanState->pBuff + 4 ), iTrackNum );
    ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );
    OS_UnlockMutex( &CmdMutex );
  }
}

// --------------------------------------------------------------------------------
// Get track name by its Id in the active group
void ACOMMAND_GetTrackName( int iTrackNum, char *pName, unsigned char iNameBuffLen )
{
  unsigned int iSize;

  // No need to do anything if name buffer isn't valid.
  if( pName == NULL || iNameBuffLen == 0 )
    return;

  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_GET_TRACK_NAME,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    WriteBE16U( pAndroidCommanState->pBuff + 4, iTrackNum );
    WriteBE16U( pAndroidCommanState->pBuff + 6, iTrackNum );

    if( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) )
    {
      if( OK == ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize ) )
      {
        unsigned char iLen =  ( pAndroidCommanState->pBuff[ 4 ] > ( iNameBuffLen - 1 ) ) ?
                              ( iNameBuffLen - 1 ) :
                              pAndroidCommanState->pBuff[ 4 ];
        memcpy( (void*)pName, (void*)( pAndroidCommanState->pBuff + 5 ), iLen );
        pName[ iLen ] = 0;
      }
    }
    OS_UnlockMutex( &CmdMutex );
  }
  else
  {
    pName[ 0 ] = 0;
  }
}

// --------------------------------------------------------------------------------
// Send controll command
void ACOMMAND_SendControl( unsigned char Msg )
{
  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_CONTROL,
                Msg,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );
    ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );
    OS_UnlockMutex( &CmdMutex );
  }
}

// --------------------------------------------------------------------------------
// Select group by ID.
// return number of items in the selected group.
unsigned int ACOMMAND_SelectGroup( int iGroupId )
{
  unsigned char  i;
  unsigned int   iSize,
                 iNumOfItems;
  signed int     rc;

  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_GET_GROUP_ITEMS_COUNT,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    pAndroidCommanState->pBuff[ 4 ] = iGroupId;

    for( i = 0; i < 10; i++ )
    {
      rc = ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );
      if( rc == OK || rc == ERR_DRIVE_NOT_PRESENT )
        break;

      OS_TaskDelay( 500/OS_TICK_RATE_MS );
    }

    if( rc == OK )
    {
      rc = ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize );
      if( rc == OK )
      {
        iNumOfItems = ReadBE32U( ( pAndroidCommanState->pBuff + 4 ) );
        OS_UnlockMutex( &CmdMutex );
        return iNumOfItems;
      }
    }
    OS_UnlockMutex( &CmdMutex );
  }
  return 0;
}

// --------------------------------------------------------------------------------
// Get number of tracks in the currently active group.
unsigned short browserGetNumOfTracks( void )
{
  unsigned int   iSize;
  unsigned short iNumOfTracks;

  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_GET_TRK_COUNT,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    if( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) )
    {
      if( OK == ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize ) )
      {
        iNumOfTracks = ReadBE32U( pAndroidCommanState->pBuff + 4 );
        OS_UnlockMutex( &CmdMutex );
        return iNumOfTracks;
      }
    }
    OS_UnlockMutex( &CmdMutex );
  }
  return 0;
}

// --------------------------------------------------------------------------------
// Get group item name by its index and group id.
// Return strlen of group item name.
unsigned short ACOMMAND_GetGroupItemName( int iGroupId,
                                          int iGroupIdx,
                                          char *pName,
                                          unsigned char iNameBuffLen )
{
  unsigned int iSize;

  // No need to do anything if name buffer isn't valid.
  if( pName == NULL || iNameBuffLen == 0 )
    return 0;

  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_GET_GROUP_ITEMS,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    pAndroidCommanState->pBuff[ 4 ] = iGroupId;
    WriteBE16U( pAndroidCommanState->pBuff + 5, iGroupIdx );
    WriteBE16U( pAndroidCommanState->pBuff + 7, iGroupIdx );

    if( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) )
    {
      if( OK == ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize ) )
      {
        unsigned char iLen =  ( pAndroidCommanState->pBuff[ 4 ] > ( iNameBuffLen - 1 ) ) ?
                              ( iNameBuffLen - 1 ) :
                              pAndroidCommanState->pBuff[ 4 ];
        memcpy( (void*)pName, (void*)( pAndroidCommanState->pBuff + 5 ), iLen );
        pName[ iLen ] = 0;
        OS_UnlockMutex( &CmdMutex );
        return iLen;
      }
    }
    OS_UnlockMutex( &CmdMutex );
  }
  else
  {
    pName[ 0 ] = 0;
  }
  return 0;
}

// --------------------------------------------------------------------------------
// Select group item by its index and group ID.
// return number of items in the selected group.
unsigned int ACOMMAND_SelectGroupItem( int iGroupId, int iGroupIdx )
{
  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_SET_ACTIVE_GROUP_ITEM,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    pAndroidCommanState->pBuff[ 4 ] = iGroupId;
    WriteBE16U( pAndroidCommanState->pBuff + 5, iGroupIdx );

    ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );
    OS_UnlockMutex( &CmdMutex );
    // Return number of tracks in activated group.
    return browserGetNumOfTracks();
  }
  return 0;
}

// --------------------------------------------------------------------------------
// Open track by Id and read tags.
// return track size if successful.
unsigned int ACOMMAND_OpenTrack( int TrackIdx, DEVICE_TRACK_INFO *trackInfo )
{
  unsigned int iSize;

  trackInfo->iTotalSize = 0;

  if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
  {
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_SET_CUR_TRK,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    WriteBE16U( pAndroidCommanState->pBuff + 4, TrackIdx );
    ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) );

    // Get Track Info
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_GET_TRK_INFO,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    WriteBE16U( pAndroidCommanState->pBuff + 4, TrackIdx );

    if( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) )
    {
      if( OK == ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize ) )
      {
        trackInfo->iTotalSize             = ReadBE32U( pAndroidCommanState->pBuff + 4 );
        trackInfo->FileType               = getExtension( (char*)(pAndroidCommanState->pBuff + 24), 3 );
        trackInfo->streamInfo.iSampleRate = ReadBE32U( pAndroidCommanState->pBuff + 8 );
        trackInfo->streamInfo.iBitRate    = ReadBE32U( pAndroidCommanState->pBuff + 13 );
        trackInfo->streamInfo.iNChan      = pAndroidCommanState->pBuff[ 12 ];
      }
    }

    // Get Track tags
    prepareCmd( MESSAGE_TYPE_MEDIA,
                MEDIA_GET_TRK_ID3_INFO,
                pAndroidCommanState->pBuff,
                pAndroidCommanState->iBuffSize );

    WriteBE16U( pAndroidCommanState->pBuff + 4, TrackIdx );
    if( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) )
    {
      if( OK == ANDROID_IN( pAndroidCommanState->pBuff, pAndroidCommanState->iBuffSize, &iSize ) )
      {
        char  *pTag = (char*)(pAndroidCommanState->pBuff + 3 );

        SET_TRACK_TEXT(  trackInfo->asTitle,
                         TRK_NAME_ID,
                         pTag + 6,
                         pTag[ 5 ] );
        pTag+= pTag[ 5 ] + 1;
        SET_TRACK_TEXT(  trackInfo->asTitle,
                         AR_NAME_ID,
                         pTag + 6,
                         pTag[ 5 ] );
        pTag+= pTag[ 5 ] + 1;
        SET_TRACK_TEXT(  trackInfo->asTitle,
                         AL_NAME_ID,
                         pTag + 6,
                         pTag[ 5 ] );
      }
    }
    OS_UnlockMutex( &CmdMutex );
  }

  if( pAndroidCommanState->bIsoAudioStreaming )
  {
    trackInfo->iTotalSize = 0x7FFFFFFF;
    pAndroidCommanState->bWavHeaderSent = FALSE;
  }

  return trackInfo->iTotalSize;
}

#if !defined(__ANDROID__) && !defined(ANDROID)
// --------------------------------------------------------------------------------
// Read data block from ISO interface
unsigned int ACOMMAND_ReadIsoData( void* buff, unsigned int iStartPos, unsigned int btr )
{
  unsigned int iWavHeaderSize = 0;
  unsigned int iSize;

  if( !pAndroidCommanState->bWavHeaderSent )
  {
    DEBUGOUT( "WAV header sent" );
    iWavHeaderSize = sizeof( wav_header_android );
    memcpy( buff, wav_header_android, iWavHeaderSize );
    pAndroidCommanState->bWavHeaderSent = TRUE;
  }

  iSize = ISO_Read( (void*)((char*)buff + iWavHeaderSize), btr - iWavHeaderSize ) + iWavHeaderSize;
  return iSize;
}

// --------------------------------------------------------------------------------
// Read data block from bulk interface
unsigned int ACOMMAND_ReadData( void* buff, unsigned int iStartPos, unsigned int btr )
{
  unsigned int iSize;

  if( pAndroidCommanState->bIsoAudioStreaming )
  {
    return ACOMMAND_ReadIsoData( buff, iStartPos, btr );
  }
  else
  {
    if( true == OS_LockMutex( &CmdMutex, ACOMMAND_MUTEX_TIMEOUT ) )
    {
      prepareCmd( MESSAGE_TYPE_BULK,
                  BULK_READ_STREAM_DATA,
                  pAndroidCommanState->pBuff,
                  pAndroidCommanState->iBuffSize );

      WriteBE32U( pAndroidCommanState->pBuff + 4, iStartPos );
      WriteBE32U( pAndroidCommanState->pBuff + 8, btr );

      if( OK == ANDROID_OUT( pAndroidCommanState->pBuff, 3 + ReadBE16U( pAndroidCommanState->pBuff + 1 ) ) )
      {
        if( OK == ANDROID_IN( buff, DECODE_BUFF_SIZE, &iSize ) )
        {
          iSize = ( iSize > btr ) ? btr : iSize;
          OS_UnlockMutex( &CmdMutex );
          return iSize;
        }
      }
      OS_UnlockMutex( &CmdMutex );
    }
    OS_TaskDelay( 50/OS_TICK_RATE_MS );  // Avoid looping in error condition.
    return 0;
  }
}
#endif

