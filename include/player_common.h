#ifndef __PLAYER_COMMON_H__
#define __PLAYER_COMMON_H__

// ---------------------------
#define MAX_TITLE_LEN     64
#define TRK_NAME_MAX_LEN  32

// ---------------------------
typedef enum
{
  TRK_NAME_ID= 0,
  AL_NAME_ID,
  AR_NAME_ID,
  MAX_NAME_ID
} TRK_TITLE_PART;

// ---------------------------
typedef enum
{
  RPT_FLAG=       0x001,
  RPT_ALL_FLAG=   0x002,
  MIX_FLAG=       0x004,
  MIX_ALL_FLAG=   0x008,
  SCAN_FLAG=      0x010,
  SCAN_DISK_FLAG= 0x020,
  FF_FLAG=        0x040,
  FR_FLAG=        0x080,
  RPT_DISK_FLAG=  0x100,
} TRACK_STATUS_FLAG;

// ---------------------------
typedef struct
{
  char          value[ TRK_NAME_MAX_LEN ];
  unsigned char len;
} TITLE_STRING;

// ---------------------------
typedef struct
{
  TRACK_STATUS_FLAG  iState;
  int                iTime;
  int                iFilePos;
  int                iTrackIndex;
  int                iGlobalTrackIndex;
  int                iCategoryIndex;
  int                iTotalSize;
  int                iTotalTime;
  MEDIA_FILE_TYPE    FileType;
  AUDIO_STREAM_INFO  streamInfo;
  TITLE_STRING       asTitle[ 3 ];
  TITLE_STRING       sFileName;
} DEVICE_TRACK_INFO;

#endif // __PLAYER_COMMON_H__

