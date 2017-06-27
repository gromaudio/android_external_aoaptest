/*
 * usbhost_iso.h
 *
 *  Created on: Jun 22, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#ifndef USBHOST_ISO_H_
#define USBHOST_ISO_H_

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

typedef void (*iso_ondata_cb)(unsigned char*, unsigned int);

signed char iso_init( iso_ondata_cb cb );
int iso_start_transfers(void);
int iso_process(void);

#endif /* USBHOST_ISO_H_ */
