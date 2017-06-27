/*
 * usbhost_hid.h
 *
 *  Created on: Jun 22, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#ifndef USBHOST_HID_H_
#define USBHOST_HID_H_

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

typedef enum
{
  HID_CMD_STOP = 0xB7,
  HID_CMD_TOGGLEPLAY = 0xCD,
  HID_CMD_NEXT = 0xB5,
  HID_CMD_PREV = 0xB6,
} HID_COMMAND;

int hid_init(void);
void hid_send( HID_COMMAND cmd );
int enable_iso_driver(void);
int disable_iso_driver(void);

#endif /* USBHOST_HID_H_ */
