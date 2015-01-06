#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "simif_zedboard.h"

#define read_reg(r) (dev_vaddr[r])
#define write_reg(r, v) (dev_vaddr[r] = v)

simif_zedboard_t::simif_zedboard_t(std::vector<std::string> args, uint64_t _max_cycles, bool _trace)
  : simif_t(args, _max_cycles, _trace)
{
  int fd = open("/dev/mem", O_RDWR|O_SYNC);
  assert(fd != -1);

  int host_prot = PROT_READ | PROT_WRITE;
  int flags = MAP_SHARED;
  uintptr_t pgsize = sysconf(_SC_PAGESIZE);
  assert(dev_paddr % pgsize == 0);

  dev_vaddr = (uintptr_t*)mmap(0, pgsize, host_prot, flags, fd, dev_paddr);
  assert(dev_vaddr != MAP_FAILED);

  // Reset
  write_reg(31, 0);
  __sync_synchronize();
  // Empty output queues before starting!
  while ((uint32_t) read_reg(0) > 0) {
    read_reg(1);
  }
}

void simif_zedboard_t::poke(uint32_t value) {
  write_reg(0, value);
  __sync_synchronize();
}

bool simif_zedboard_t::peek_ready() {
  return (uint32_t) read_reg(0) != 0;
}

uint32_t simif_zedboard_t::peek() {
  __sync_synchronize();
  while (!peek_ready()) ;
  return (uint32_t) read_reg(1);
}

void simif_zedboard_t::poke_htif(uint32_t value) {
  write_reg(2, value);
  __sync_synchronize();
}

bool simif_zedboard_t::peek_htif_ready() {
  return (uint32_t) read_reg(2) != 0;
}

uint32_t simif_zedboard_t::peek_htif() {
  __sync_synchronize();
  while (!peek_ready()) ;
  return (uint32_t) read_reg(3);
}

void simif_zedboard_t::step_htif() {
  size_t size = hostlen / 8;
  while (from_htif.size() >= size) {
    uint32_t value;
    std::copy(from_htif.begin(), from_htif.begin() + size, &value);
    poke_htif(value);
  }
  while (peek_htif_ready()) {
    uint32_t value = peek_htif();
    to_htif.insert(to_htif.end(), (const char*)&value, (const char*)&value + size);
  }
}
