#ifndef __USBHOSTBULK_H__
#define __USBHOSTBULK_H__

#if defined(__ANDROID__) || defined(ANDROID)
  #define  OK                        0
  #define  ERR_TD_FAIL              -1
  #define  ERR_ENUM_FAILED          -2
  #define  ERR_INIT_FAILED          -3
  #define  ERR_BAD_CONFIGURATION    -4
  #define  ERR_DRIVE_NOT_PRESENT    -5
  #define  ERR_DRIVE_NOT_READY      -6
  #define  ERR_ENUM_AGAIN           -7
  #define  ERR_TIMEOUT              -8
#endif

signed char Bulk_DriveInsert( void );
signed char Bulk_OUT( volatile unsigned char *aBuff, unsigned short iSize );
signed char Bulk_IN( volatile unsigned char *aBuff, unsigned short iSize, unsigned int *piBytesRead );


#endif /* __USBHOSTBULK_H__ */
