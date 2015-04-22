#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"

//---------------------------------------------------------------------------
static const unsigned int EXTENSIONS[]= {  0x4D503300 /*MP3*/ | MP3_FILE_TYPE,
                                           0x6D703300 /*mp3*/ | MP3_FILE_TYPE,
                                           0x57415600 /*WAV*/ | WAV_FILE_TYPE,
                                           0x77617600 /*wav*/ | WAV_FILE_TYPE,
                                           0x464C4300 /*FLC*/ | FLAC_FILE_TYPE,
                                           0x464C4100 /*FLA*/ | FLAC_FILE_TYPE,
                                           0x574D4100 /*WMA*/ | WMA_FILE_TYPE,
                                           0x4F474700 /*OGG*/ | OGG_FILE_TYPE,
                                           0x41414300 /*AAC*/ | AAC_FILE_TYPE,
                                           0x4D344100 /*M4A*/ | MP4_FILE_TYPE,
                                           0x4D344200 /*M4B*/ | MP4_FILE_TYPE,
                                           0x4D345000 /*M4P*/ | MP4_FILE_TYPE
                                         };

//---------------------------------------------------------------------------
MEDIA_FILE_TYPE getExtension( char* sFileName, int iLen )
{
  unsigned char i;
  unsigned int iExtension= sFileName[ iLen - 3 ] << 24 |
                           sFileName[ iLen - 2 ] << 16 |
                           sFileName[ iLen - 1 ] << 8;

  for( i= 0; i< sizeof( EXTENSIONS ); i++ )
  {
    if( iExtension== ( EXTENSIONS[ i ] & 0xFFFFFF00 ) )
      return (MEDIA_FILE_TYPE)(EXTENSIONS[ i ] & 0xFF);
  }

  return UNKNOWN_FILE_TYPE;
}

//---------------------------------------------------------------------------
unsigned char safeStrCpy( char *sDest,
                          unsigned char iDestSize,
                          char* sSrc,
                          unsigned char iSrcSize )
{
  if( iSrcSize >= iDestSize )
    iSrcSize = iDestSize - 1;

  strncpy( sDest, sSrc, iSrcSize );
  sDest[ iSrcSize ] = 0;
  return strlen( sDest );
}

//---------------------------------------------------------------------------
void delayUs(unsigned int time_us)
{
  usleep(time_us);
}

//---------------------------------------------------------------------------
void  WriteLE32U (volatile  unsigned char  *pmem,
                            unsigned int   val)
{
  pmem[0] = ((unsigned char *)&val)[0];
  pmem[1] = ((unsigned char *)&val)[1];
  pmem[2] = ((unsigned char *)&val)[2];
  pmem[3] = ((unsigned char *)&val)[3];
}

//---------------------------------------------------------------------------
unsigned short ReadLE16U (volatile unsigned char *pmem)
{
  unsigned short   val;

  ((unsigned char *)&val)[0] = pmem[0];
  ((unsigned char *)&val)[1] = pmem[1];

  return (val);
}

//---------------------------------------------------------------------------
void  WriteLE16U (volatile  unsigned char  *pmem,
                            unsigned short   val)
{
  pmem[0] = ((unsigned char *)&val)[0];
  pmem[1] = ((unsigned char *)&val)[1];
}

//---------------------------------------------------------------------------
unsigned int  ReadBE32U (volatile  unsigned char  *pmem)
{
  unsigned int   val = 0;

  ((unsigned char *)&val)[0] = pmem[3];
  ((unsigned char *)&val)[1] = pmem[2];
  ((unsigned char *)&val)[2] = pmem[1];
  ((unsigned char *)&val)[3] = pmem[0];

  return (val);
}

//---------------------------------------------------------------------------
void WriteBE32U( volatile unsigned char *pmem,
                          unsigned int   val)
{
  pmem[0] = ((unsigned char *)&val)[3];
  pmem[1] = ((unsigned char *)&val)[2];
  pmem[2] = ((unsigned char *)&val)[1];
  pmem[3] = ((unsigned char *)&val)[0];
}

//---------------------------------------------------------------------------
unsigned short ReadBE16U( volatile unsigned char *pmem)
{
  unsigned short val;

  ((unsigned char *)&val)[0] = pmem[1];
  ((unsigned char *)&val)[1] = pmem[0];

  return (val);
}

//---------------------------------------------------------------------------
void WriteBE16U( volatile unsigned char  *pmem,
                          unsigned short  val)
{
  pmem[0] = ( val >> 8 ) & 0xFF;
  pmem[1] = val & 0xFF;
}

//---------------------------------------------------------------------------
void __assert(const char *file, int line, const char *failedexpr)
{
  (void)fprintf(stderr, "assertion \"%s\" failed: file \"%s\", line %d\n",
                failedexpr,
                file,
                line);
  abort();
/* NOTREACHED */
}



