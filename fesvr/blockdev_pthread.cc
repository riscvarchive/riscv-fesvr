#include "blockdev_pthread.h"

static void blockdev_thread_main(void *arg)
{
  blockdev_pthread_t *bdev = static_cast<blockdev_pthread_t*>(arg);
  bdev->run();

  while (true)
    bdev->switch_to_target();
}

blockdev_pthread_t::blockdev_pthread_t(
    const std::vector<std::string>& args)
  : blockdev_t(args)
{
  target = context_t::current();
  host.init(blockdev_thread_main, this);
}

blockdev_pthread_t::~blockdev_pthread_t()
{
}

void blockdev_pthread_t::send(uint64_t data)
{
  th_data.push_back(data);
}

bool blockdev_pthread_t::recv_nonblocking(uint64_t *data)
{
  if (ht_data.empty()) {
    host.switch_to();
    return false;
  }

  *data = ht_data.front();
  ht_data.pop_front();

  return true;
}

uint64_t blockdev_pthread_t::recv(void)
{
  uint64_t data;

  while (!recv_nonblocking(&data));

  return data;
}

uint64_t blockdev_pthread_t::poll(void)
{
  uint64_t data;

  while (th_data.empty())
    target->switch_to();

  data = th_data.front();
  th_data.pop_front();

  return data;
}

void blockdev_pthread_t::push(uint64_t data)
{
  ht_data.push_back(data);
}
