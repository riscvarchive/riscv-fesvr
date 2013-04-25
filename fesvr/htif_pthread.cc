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
  : htif_t(args), kill(false)
{
  assert(pthread_mutex_init(&th_lock, 0) == 0);
  assert(pthread_cond_init(&th_cond, 0) == 0);
  assert(pthread_mutex_init(&ht_lock, 0) == 0);
  assert(pthread_cond_init(&ht_cond, 0) == 0);
  assert(pthread_mutex_init(&stop_once, 0) == 0);
  assert(pthread_create(&host, 0, thread_main, this) == 0);
}

htif_pthread_t::~htif_pthread_t()
{
  kill = true;
  send(0, 0); // wake up host thread if blocked on read
  pthread_join(host, 0);
}

ssize_t htif_pthread_t::read(void* buf, size_t max_size)
{
  pthread_mutex_lock(&th_lock);
  if (th_data.empty() && !kill)
    pthread_cond_wait(&th_cond, &th_lock);
  size_t s = std::min(max_size, th_data.size());
  std::copy(th_data.begin(), th_data.begin() + s, (char*)buf);
  th_data.erase(th_data.begin(), th_data.begin() + s);
  pthread_mutex_unlock(&th_lock);
  if (kill)
    pthread_exit(0);
  return s;
}

ssize_t htif_pthread_t::write(const void* buf, size_t size)
{
  pthread_mutex_lock(&ht_lock);
  ht_data.insert(ht_data.end(), (const char*)buf, (const char*)buf + size);
  pthread_cond_signal(&ht_cond);
  pthread_mutex_unlock(&ht_lock);
  return size;
}

void htif_pthread_t::send(const void* buf, size_t size)
{
  pthread_mutex_lock(&th_lock);
  th_data.insert(th_data.end(), (const char*)buf, (const char*)buf + size);
  pthread_cond_signal(&th_cond);
  pthread_mutex_unlock(&th_lock);
}

void htif_pthread_t::recv(void* buf, size_t size)
{
  pthread_mutex_lock(&ht_lock);
  while (ht_data.size() < size)
    pthread_cond_wait(&ht_cond, &ht_lock);
  std::copy(ht_data.begin(), ht_data.begin() + size, (char*)buf);
  ht_data.erase(ht_data.begin(), ht_data.begin() + size);
  pthread_mutex_unlock(&ht_lock);
}

bool htif_pthread_t::recv_nonblocking(void* buf, size_t size)
{
  pthread_mutex_lock(&ht_lock);
  bool ret = ht_data.size() >= size;
  if (ret)
  {
    std::copy(ht_data.begin(), ht_data.begin() + size, (char*)buf);
    ht_data.erase(ht_data.begin(), ht_data.begin() + size);
  }
  pthread_mutex_unlock(&ht_lock);
  return ret;
}
