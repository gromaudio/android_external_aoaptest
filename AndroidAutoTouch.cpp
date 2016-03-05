#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>
#include "AndroidAutoTouch.h"

//--------------------------------------------------------------------------
#define AA_CH_TOUCH           3
#define ACTION_DOWN           0
#define ACTION_UP             1
#define ACTION_MOVE           2

//--------------------------------------------------------------------------
typedef struct
{
  bool  NeedsReport;
  bool  NewButton;
  bool  OldButton;
  int   X;
  int   Y;
}TOUCH_EVENT;

const uint8_t ba_touch[] = { AA_CH_TOUCH, 0x0b, 0x03, 0x00, 0x80, 0x01,
                             0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0, 0,
                             0x1a, 0x0e,
                             0x0a, 0x08,
                             0x08, 0x2e, 0,
                             0x10, 0x2b, 0,
                             0x18, 0x00,
                             0x10, 0x00,
                             0x18, 0x00 };

int           touchFd;
TOUCH_EVENT   event;

//--------------------------------------------------------------------------
void TOUCH_init(const char* touch_dev_path)
{
  event.NeedsReport = false;
  event.NewButton   = false;
  event.OldButton   = false;
  event.X           = 0;
  event.Y           = 0;

  touchFd = open(touch_dev_path, O_RDONLY | O_NONBLOCK);
  if(touchFd == -1)
    fprintf(stderr, "Cannot open input device %s: %d, %s\n", touch_dev_path, errno, strerror(errno));

  fprintf(stderr, "Event device opened: %s\n", touch_dev_path);
}

//--------------------------------------------------------------------------
static size_t varint_encode_32(uint32_t val, uint8_t *ba, size_t idx)
{
  if(val >= 1 << 14)
  {
//    fprintf(stderr, "32: Too big\n");
    return 1;
  }

  ba[ idx + 0 ] = (uint8_t)(0x7f & (val >> 0));
  ba[ idx + 1 ] = (uint8_t)(0x7f & (val >> 7));
  if(ba[ idx + 1 ] != 0)
  {
    ba[ idx + 0 ] |= 0x80;
    return 2;
  }
  return 1;
}

//--------------------------------------------------------------------------
static size_t varint_encode_64(uint64_t val, uint8_t *ba, size_t idx)
{
  if(val >= 0x7fffffffffffffffL)
  {
//    fprintf(stderr, "64: Too big\n");
    return 1;
  }

  uint64_t left = val;
  for(int idx2 = 0; idx2 < 9; idx2 ++)
  {
    ba[ idx + idx2 ] = (uint8_t)(0x7f & left);
    left = left >> 7;
    if(left == 0)
    {
      return idx2 + 1;
    }
    else if(idx2 < 9 - 1)
    {
      ba[ idx + idx2 ] |= 0x80;
    }
  }
  return 9;
}

//--------------------------------------------------------------------------
static size_t processEvent(uint8_t action, TOUCH_EVENT *ev, uint8_t *buff, size_t buff_size)
{
  size_t   siz_arr, idx, size1_idx, size2_idx;
  uint64_t ts;
  struct timeval tv;

  gettimeofday( &tv, NULL );
  ts = 1000000000 * tv.tv_sec + 1000 * tv.tv_usec;
  memcpy(buff, ba_touch, sizeof(ba_touch));

  siz_arr = 0;

  idx = 1+6 + varint_encode_64(ts, buff, 1+6);          // Encode timestamp

  buff[ idx++ ] = 0x1a;                                 // Value 3 array
  size1_idx = idx;                                      // Save size1_idx
  buff[ idx++ ] = 0x0a;                                 // Default size 10

  buff[ idx++ ] = 0x0a;                                 // Contents = 1 array
  size2_idx = idx;                                      // Save size2_idx
  buff[ idx++ ] = 0x04;                                 // Default size 4

  buff[ idx++ ] = 0x08;                                 // Value 1
  siz_arr = varint_encode_32(ev->X, buff, idx);         // Encode X
  idx += siz_arr;
  buff[ size1_idx ] += siz_arr;                         // Adjust array sizes for X
  buff[ size2_idx ] += siz_arr;

  buff[ idx++ ] = 0x10;                                 // Value 2
  siz_arr = varint_encode_32(ev->Y, buff, idx);         // Encode Y
  idx += siz_arr;
  buff[ size1_idx ] += siz_arr;                         // Adjust array sizes for Y
  buff[ size2_idx ] += siz_arr;

  buff[ idx++ ] = 0x18;                                 // Value 3
  buff[ idx++ ] = 0x00;                                 // Encode Z ?

  buff[ idx++ ] = 0x10;
  buff[ idx++ ] = 0x00;

  buff[ idx++ ] = 0x18;
  buff[ idx++ ] = action;

  return idx;
}

//--------------------------------------------------------------------------
size_t TOUCH_getAction(uint8_t *buff, size_t buff_size)
{
  struct input_event ev;
  size_t event_data_size = 0;

  if(touchFd == -1)
    return event_data_size;

  if(read(touchFd, &ev, sizeof(ev)) == sizeof(ev))
  {
    fprintf(stderr, "Event: %d, %d, %d\n", ev.type, ev.code, ev.value);

    switch(ev.type)
    {
      case EV_SYN:
        if(event.NeedsReport)
        {
          if(event.NewButton && !event.OldButton)
          {
//            fprintf(stderr, "Event: DOWN %d, %d\n", event.X, event.Y);
            event_data_size = processEvent(ACTION_DOWN, &event, buff, buff_size);
          }

          if(!event.NewButton && event.OldButton)
          {
//            fprintf(stderr, "Event: UP %d, %d\n", event.X, event.Y);
            event_data_size = processEvent(ACTION_UP, &event, buff, buff_size);
          }

          if(event.NewButton == event.OldButton)
          {
//            fprintf(stderr, "Event: MOVE %d, %d\n", event.X, event.Y);
            event_data_size = processEvent(ACTION_MOVE, &event, buff, buff_size);
          }

          event.OldButton   = event.NewButton;
          event.NeedsReport = false;
        }
        break;

      case EV_KEY:
        event.NewButton = ev.value ? true : false;
        event.NeedsReport = true;
        break;

      case EV_ABS:
        if(ev.code == ABS_X)
        {
          event.X = ev.value * 800 / 255;
          event.NeedsReport = true;
        }

        if(ev.code == ABS_Y)
        {
          event.Y = ev.value * 480 / 255;
          event.NeedsReport = true;
        }
        break;
    }
  }

  return event_data_size;
}
