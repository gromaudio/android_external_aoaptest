#ifndef __ANDROID_AUTO_MIC_H__
#define __ANDROID_AUTO_MIC_H__

#include <sys/types.h>

void   MIC_init( void );
size_t MIC_getData( uint8_t *buff, size_t buff_size );

#endif /* __ANDROID_AUTO_MIC_H__ */
