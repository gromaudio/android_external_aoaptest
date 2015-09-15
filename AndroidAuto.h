#ifndef __AUTO_VIDEO_H__
#define __AUTO_VIDEO_H__

#include <utils/Errors.h>
#include <sys/types.h>

using namespace android;

status_t AUTO_init( void );
status_t AUTO_tick( void );
void     AUTO_exit( void );

#endif
