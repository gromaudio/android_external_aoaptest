#ifndef __AUTO_TOUCH_h__
#define __AUTO_TOUCH_h__

#include <sys/types.h>

void   TOUCH_init(const char* touch_dev_path);
size_t TOUCH_getAction(uint8_t *buff, size_t buff_size);




#endif
