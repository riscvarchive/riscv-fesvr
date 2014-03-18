#ifndef _HTIF_CONTEXT_H
#define _HTIF_CONTEXT_H

// A replacement for ucontext.h, which is sadly deprecated.

#include <pthread.h>

#if defined(__GLIBC__)
# undef USE_UCONTEXT
# define USE_UCONTEXT
# include <ucontext.h>
# include <memory>
#endif

class context_t
{
 public:
  context_t();
  ~context_t();
  void init(void (*func)(void*), void* arg);
  void switch_to();
  static context_t* current();
 private:
  context_t* creator;
  void (*func)(void*);
  void* arg;
#ifdef USE_UCONTEXT
  std::unique_ptr<ucontext_t> context;
  static void wrapper(context_t*);
#else
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  volatile int flag;
  static void* wrapper(void*);
#endif
};

#endif
