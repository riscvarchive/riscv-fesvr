// See LICENSE for license details.

#include "htif_ucontext.h"
#include <algorithm>
#include <stdio.h>

static void thread_main(htif_ucontext_t* htif)
{
  htif->run();
}

htif_ucontext_t::htif_ucontext_t(const std::vector<std::string>& args)
  : htif_t(args)
{
  assert(getcontext(&host) == 0);
  host.uc_link = NULL;
  host.uc_stack.ss_size = 64*1024;
  host.uc_stack.ss_sp = new void*[host.uc_stack.ss_size/sizeof(void*)];
  makecontext(&host, (void(*)(void))thread_main, 1, this);
}

htif_ucontext_t::~htif_ucontext_t()
{
  delete [] (void**)host.uc_stack.ss_sp;
}

ssize_t htif_ucontext_t::read(void* buf, size_t max_size)
{
  while (th_data.size() == 0)
    assert(swapcontext(&host, &target) == 0);
    
  size_t s = std::min(max_size, th_data.size());
  std::copy(th_data.begin(), th_data.begin() + s, (char*)buf);
  th_data.erase(th_data.begin(), th_data.begin() + s);

  return s;
}

ssize_t htif_ucontext_t::write(const void* buf, size_t size)
{
  ht_data.insert(ht_data.end(), (const char*)buf, (const char*)buf + size);
  return size;
}

void htif_ucontext_t::send(const void* buf, size_t size)
{
  th_data.insert(th_data.end(), (const char*)buf, (const char*)buf + size);
}

void htif_ucontext_t::recv(void* buf, size_t size)
{
  while (!this->recv_nonblocking(buf, size))
    ;
}

bool htif_ucontext_t::recv_nonblocking(void* buf, size_t size)
{
  if (ht_data.size() < size)
  {
    assert(swapcontext(&target, &host) == 0);
    return false;
  }

  std::copy(ht_data.begin(), ht_data.begin() + size, (char*)buf);
  ht_data.erase(ht_data.begin(), ht_data.begin() + size);
  return true;
}
