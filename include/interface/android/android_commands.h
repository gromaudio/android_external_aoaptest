#ifndef __ANDROID_COMMANDS_H__
#define __ANDROID_COMMANDS_H__

#include <stdbool.h>
#include "player_common.h"

#define ANDROID_BUFF_SIZE       512

typedef struct
{
  unsigned char     pBuff[ ANDROID_BUFF_SIZE ];
  unsigned int      iBuffSize;
  bool              bIsoAudioStreaming;
  bool              bWavHeaderSent;
}ANDROID_COMMAND_STATE;

void           ACOMMAND_Init( void );
void           ACOMMAND_Activate( ANDROID_COMMAND_STATE *pState );
void           ACOMMAND_SelectCurrentTrack( int iTrackNum );
void           ACOMMAND_GetDeviceName( char *pName, unsigned char iNameSize );
void           ACOMMAND_GetTrackName( int iTrackNum, char *pName, unsigned char iNameSize );
unsigned int   ACOMMAND_OpenTrack( int TrackIdx, DEVICE_TRACK_INFO *trackInfo );
unsigned int   ACOMMAND_SelectGroup( int iGroupId );
void           ACOMMAND_Seek( unsigned int iPosMS );
unsigned int   ACOMMAND_SelectGroupItem( int iGroupId, int iGroupIdx );
unsigned short ACOMMAND_GetGroupItemName( int iGroupId,
                                          int iGroupIdx,
                                          char *pName,
                                          unsigned char iNameBuffLen );
void           ACOMMAND_SendControl( unsigned char Msg );
void           ACOMMAND_SendStatus( unsigned int   iTime,
                                    unsigned char  iPlaybackStatus,
                                    unsigned short iPlaybackFlags );
unsigned int   ACOMMAND_ReadStatus( unsigned char *pBuff,
                                    unsigned int iBuffSize );
unsigned int   ACOMMAND_ReadData( void* buff,
                                  unsigned int iStartPos,
                                  unsigned int btr );





#endif /* __ANDROID_COMMANDS_H__ */

