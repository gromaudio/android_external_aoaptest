#include "common.h"

//---------------------------------------------------------------------------
void OS_TaskDelay(unsigned int delayMs)
{
  delayUs(1000*delayMs);
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
  int timeoutUs = timeoutMs * 1000;
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


