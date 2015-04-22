#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "os.h"

unsigned char safeStrCpy( char         *sDest,
                          unsigned char iDestSize,
                          char         *sSrc,
                          unsigned char iSrcSize );


#define FALSE  0
#define TRUE   1
#define GET_MIN_VALUE( x, y )   ( ( x < y ) ? x : y )
#define GET_MAX_VALUE( x, y )   ( ( x > y ) ? x : y )
#define NELEMS(x)               (sizeof(x) / sizeof(x[0]))
#define CHECK_BIT(var,pos)      ((var & (1 << pos)) == (1 << pos))

#define SET_TRACK_TEXT( asTitle, id, v, l )           \
  asTitle[ id ].len= safeStrCpy( asTitle[ id ].value, \
                                  TRK_NAME_MAX_LEN,   \
                                  v,                  \
                                  l )
#define GET_TRACK_TEXT( asTitle, id ) asTitle[ id ].value

typedef struct
{
  int  iSampleRate;
  int  iBitRate;
  char iNChan;
} AUDIO_STREAM_INFO;

typedef enum
{
  UNKNOWN_FILE_TYPE,
  MP3_FILE_TYPE,
  WMA_FILE_TYPE,
  AAC_FILE_TYPE,
  OGG_FILE_TYPE,
  WAV_FILE_TYPE,
  FLAC_FILE_TYPE,
  MP4_FILE_TYPE
}MEDIA_FILE_TYPE;

#define configASSERT(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

MEDIA_FILE_TYPE getExtension( char* sFileName, int iLen );
unsigned long long getTimestamp(void);
void               delayUs(unsigned int time_us);
unsigned int    ReadLE32U     (volatile  unsigned char  *pmem);
void            WriteLE32U    (volatile  unsigned char  *pmem,
                                         unsigned int    val);
unsigned short  ReadLE16U     (volatile  unsigned char  *pmem);
void            WriteLE16U    (volatile  unsigned char  *pmem,
                                         unsigned short  val);
unsigned int    ReadBE32U     (volatile  unsigned char  *pmem);
void            WriteBE32U    (volatile  unsigned char  *pmem,
                                         unsigned int    val);
unsigned short  ReadBE16U     (volatile  unsigned char  *pmem);
void            WriteBE16U    (volatile  unsigned char  *pmem,
                                         unsigned short  val);
void __assert(const char *file, int line, const char *failedexpr);


#endif /* __COMMON_H__ */
