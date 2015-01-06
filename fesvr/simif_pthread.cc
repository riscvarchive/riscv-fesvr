#include "simif_pthread.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>

void simif_pthread_t::thread_main(void *arg) {
  simif_pthread_t* simif = static_cast<simif_pthread_t*>(arg);
  simif->run();
  while (true)
    simif->target->switch_to();
}

simif_pthread_t::simif_pthread_t(std::vector<std::string> args, uint64_t _max_cycles, bool _trace)
  : simif_t(args, _max_cycles, _trace)
{
  target = context_t::current();
  host.init(thread_main, this);
}

void simif_pthread_t::poke(uint32_t value) {
  ht_data.push(value);
}

bool simif_pthread_t::peek_ready() {
  return th_data.size() > 0;
}

uint32_t simif_pthread_t::peek() {
  while (!peek_ready())
    target->switch_to();
  uint32_t value = th_data.front();
  th_data.pop();
  return value;
}

void simif_pthread_t::step_htif() {
  target->switch_to();
}

void simif_pthread_t::send(uint32_t value) {
  th_data.push(value);
}

uint32_t simif_pthread_t::recv() {
  uint32_t value;
  while(!recv_nonblocking(value)) ;
  return value;
}

bool simif_pthread_t::recv_nonblocking(uint32_t& value) {
  if (ht_data.empty()) {
    host.switch_to();
    return false;
  }
  value = ht_data.front();
  ht_data.pop();
  return true;
}

void simif_pthread_t::send_to_htif(const void* buf, size_t size) {
  to_htif.insert(to_htif.end(), (const char*)buf, (const char*)buf + size);
}

void simif_pthread_t::recv_from_htif(void *buf, size_t size) {
  while (!this->recv_from_htif_nonblocking(buf, size));
}

bool simif_pthread_t::recv_from_htif_nonblocking(void *buf, size_t size) {
  if (from_htif.size() < size) {
    host.switch_to();
    return false;
  }
 
  std::copy(from_htif.begin(), from_htif.begin() + size, (unsigned char*)buf);
  from_htif.erase(from_htif.begin(), from_htif.begin() + size);
  return true;
}
