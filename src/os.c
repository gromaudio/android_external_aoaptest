/*****************************************************************************
 *   os.c:  OS abstraction layer.
 *
 *   Copyright(C) 2015, X-mediatech
 *   All rights reserved.
 *
 *   Authors: Ivan Zaitsev
 *
******************************************************************************/

#include "os.h"

#if defined(__ANDROID__) || defined(ANDROID)
#include <unistd.h>
//---------------------------------------------------------------------------
void OS_TaskDelay(unsigned int delayMs)
{
  usleep(1000*delayMs);
}

//---------------------------------------------------------------------------
OS_TICK_TYPE OS_GetTickCount(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (OS_TICK_TYPE)(tv.tv_sec) * 1000 +
         (OS_TICK_TYPE)(tv.tv_usec) / 1000;
}

//---------------------------------------------------------------------------
bool OS_CreateMutex(OS_MUTEX_TYPE *mutex)
{
  if(pthread_mutex_init(mutex, NULL) != 0)
    return false;
  return true;
}

//---------------------------------------------------------------------------
bool OS_LockMutex(OS_MUTEX_TYPE *mutex, unsigned int timeoutMs)
{
  int timeoutUs;

  if( timeoutMs == 0 )
  {
    if( pthread_mutex_trylock(mutex) == 0 )
      return true;
    else
      return false;
  }

  timeoutUs = timeoutMs * 1000;
  while( (timeoutUs > 0) && (pthread_mutex_trylock(mutex) != 0) )
  {
    usleep(1000);
    timeoutUs -= 1000;
  }
  if(timeoutUs > 0)
    return true;

  return false;
}
//---------------------------------------------------------------------------
void OS_UnlockMutex(OS_MUTEX_TYPE *mutex)
{
  pthread_mutex_unlock(mutex);
}

#else
//---------------------------------------------------------------------------
void OS_TaskDelay(unsigned int delayMs)
{
  vTaskDelay( delayMs );
}

//---------------------------------------------------------------------------
OS_TICK_TYPE OS_GetTickCount(void)
{
  return xTaskGetTickCount();
}

//---------------------------------------------------------------------------
bool OS_CreateMutex(OS_MUTEX_TYPE *mutex)
{
  *mutex = xSemaphoreCreateMutex();
  if( *mutex != NULL )
    return true;
  else
    return false;
}

//---------------------------------------------------------------------------
bool OS_LockMutex(OS_MUTEX_TYPE *mutex, unsigned int timeoutMs)
{
  if( pdTRUE == xSemaphoreTake( *mutex, timeoutMs ) )
    return true;
  else
    return false;
}
//---------------------------------------------------------------------------
void OS_UnlockMutex(OS_MUTEX_TYPE *mutex)
{
  xSemaphoreGive( *mutex );
}
#endif

