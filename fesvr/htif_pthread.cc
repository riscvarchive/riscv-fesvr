// See LICENSE for license details.

#include "htif_pthread.h"
#include <algorithm>
#include <stdio.h>

static void* thread_main(void* arg)
{
  htif_pthread_t* htif = static_cast<htif_pthread_t*>(arg);
  htif->run();
  return 0;
}

htif_pthread_t::htif_pthread_t(const std::vector<std::string>& args)
  : htif_t(args), kill(false), lock0(PTHREAD_MUTEX_INITIALIZER),
    lock1(PTHREAD_MUTEX_INITIALIZER)
{
  assert(pthread_create(&host, 0, thread_main, this) == 0);
}

htif_pthread_t::~htif_pthread_t()
{
  // wake up host thread if blocked
  kill = true;
  pthread_mutex_trylock(&lock1);
  pthread_mutex_unlock(&lock1);
}

ssize_t htif_pthread_t::read(void* buf, size_t max_size)
{
  pthread_mutex_lock(&lock1);
  if (kill)
    pthread_exit(0);
  size_t s = std::min(max_size, th_data.size());
  if (s > 0)
  {
    std::copy(th_data.begin(), th_data.begin() + s, (char*)buf);
    th_data.erase(th_data.begin(), th_data.begin() + s);
  }
  pthread_mutex_unlock(&lock0);
  return s;
}

ssize_t htif_pthread_t::write(const void* buf, size_t size)
{
  pthread_mutex_lock(&lock1);
  if (kill)
    pthread_exit(0);
  ht_data.insert(ht_data.end(), (const char*)buf, (const char*)buf + size);
  pthread_mutex_unlock(&lock0);
  return size;
}

void htif_pthread_t::send(const void* buf, size_t size)
{
  pthread_mutex_lock(&lock0);
  th_data.insert(th_data.end(), (const char*)buf, (const char*)buf + size);
  pthread_mutex_unlock(&lock1);
}

void htif_pthread_t::recv(void* buf, size_t size)
{
  while (!this->recv_nonblocking(buf, size))
    ;
}

bool htif_pthread_t::recv_nonblocking(void* buf, size_t size)
{
  pthread_mutex_lock(&lock0);
  bool ret = ht_data.size() >= size;
  if (ret)
  {
    std::copy(ht_data.begin(), ht_data.begin() + size, (char*)buf);
    ht_data.erase(ht_data.begin(), ht_data.begin() + size);
  }
  pthread_mutex_unlock(&lock1);
  return ret;
}
