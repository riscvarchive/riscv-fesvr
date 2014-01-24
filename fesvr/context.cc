#include "context.h"
#include <assert.h>
#include <sched.h>

static thread_local context_t* cur;

context_t::context_t()
  : mutex(PTHREAD_MUTEX_INITIALIZER),
    cond(PTHREAD_COND_INITIALIZER),
    creator(NULL), func(NULL), arg(NULL), flag(0)
{
}

void* context_t::wrapper(void* a)
{
  context_t* ctx = static_cast<context_t*>(a);
  cur = ctx;
  ctx->creator->switch_to();

  ctx->func(ctx->arg);
  return NULL;
}

void context_t::init(void (*f)(void*), void* a)
{
  assert(flag == 0);
  func = f;
  arg = a;
  creator = current();

  pthread_mutex_lock(&creator->mutex);
  creator->flag = 0;
  assert(pthread_create(&thread, NULL, &context_t::wrapper, this) == 0);
  while (!creator->flag)
    pthread_cond_wait(&creator->cond, &creator->mutex);
  pthread_mutex_unlock(&creator->mutex);
}

context_t::~context_t()
{
  assert(pthread_join(thread, NULL) == 0);
}

void context_t::switch_to()
{
  assert(this != cur);
  cur->flag = 0;
  this->flag = 1;
  pthread_mutex_lock(&this->mutex);
  pthread_cond_signal(&this->cond);
  pthread_mutex_unlock(&this->mutex);
  pthread_mutex_lock(&cur->mutex);
  while (!cur->flag)
    pthread_cond_wait(&cur->cond, &cur->mutex);
  pthread_mutex_unlock(&cur->mutex);
}

context_t* context_t::current()
{
  if (cur == NULL)
  {
    cur = new context_t;
    cur->thread = pthread_self();
    cur->flag = 1;
  }
  return cur;
}
