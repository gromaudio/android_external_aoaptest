#ifndef __OS_H__
#define __OS_H__

#if defined(__ANDROID__) || defined(ANDROID)
  #include <stdio.h>
  #include <stdbool.h>
  #include <pthread.h>

  typedef unsigned long long portTickType;

  #define OS_TICK_RATE_MS         1
  #define OS_TICK_TYPE            unsigned long long
  #define OS_MUTEX_TYPE           pthread_mutex_t

#else
  #error "ERROR! Define OS functions wrapper for default architecture."
#endif

void          OS_TaskDelay(unsigned int delayMs);
OS_TICK_TYPE  OS_GetTickCount(void);

bool          OS_CreateMutex(OS_MUTEX_TYPE *mutex);
bool          OS_LockMutex(OS_MUTEX_TYPE *mutex, unsigned int timeoutMs);
void          OS_UnlockMutex(OS_MUTEX_TYPE *mutex);

#endif /* __OS_H__ */
