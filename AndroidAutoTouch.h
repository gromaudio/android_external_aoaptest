#ifndef __ANDROID_AUTO_TOUCH_H__
#define __ANDROID_AUTO_TOUCH_H__

#include <sys/types.h>

void   TOUCH_init( const char* touch_dev_path );
size_t TOUCH_getAction( uint8_t *buff, size_t buff_size );

#endif /* __ANDROID_AUTO_TOUCH_H__ */
