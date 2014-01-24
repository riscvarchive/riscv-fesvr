#ifndef _HTIF_CONTEXT_H
#define _HTIF_CONTEXT_H

// A replacement for ucontext.h, which is sadly deprecated.

#include <pthread.h>

class context_t
{
 public:
  context_t();
  ~context_t();
  void init(void (*func)(void*), void* arg);
  void switch_to();
  static context_t* current();
 private:
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  context_t* creator;
  void (*func)(void*);
  void* arg;
  volatile int flag;
  static void* wrapper(void*);
};

#endif
