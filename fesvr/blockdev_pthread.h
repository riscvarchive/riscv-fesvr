#ifndef __BLOCKDEV_PTHREAD_H__
#define __BLOCKDEV_PTHREAD_H__

#include "blockdev.h"
#include "context.h"
#include <deque>
#include <vector>
#include <string>

class blockdev_pthread_t : public blockdev_t
{
 public:
  blockdev_pthread_t(const std::vector<std::string>& target_args);
  virtual ~blockdev_pthread_t();

  void send(uint64_t data);
  uint64_t recv();
  bool recv_nonblocking(uint64_t *data);

  void switch_to_target(void) { this->target->switch_to(); }

protected:
  virtual uint64_t poll(void);
  virtual void push(uint64_t val);

 private:
  std::deque<uint64_t> th_data;
  std::deque<uint64_t> ht_data;
  context_t host;
  context_t* target;
};

#endif
