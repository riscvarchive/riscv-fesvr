#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "simif_zedboard.h"

#define read_reg(r) (dev_vaddr[r])
#define write_reg(r, v) (dev_vaddr[r] = v)

simif_zedboard_t::simif_zedboard_t(
  std::vector<std::string> args, 
  std::string prefix, 
  bool log, 
  bool check_sample, 
  bool has_htif)
  : simif_t(args, prefix, log, check_sample, has_htif)
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
  if (has_htif) {
    while ((uint32_t) read_reg(2) > 0) {
      read_reg(3);
    }
  }
}

void simif_zedboard_t::poke_host(uint32_t value) {
  write_reg(0, value);
  __sync_synchronize();
}

bool simif_zedboard_t::peek_host_ready() {
  return (uint32_t) read_reg(0) != 0;
}

uint32_t simif_zedboard_t::peek_host() {
  __sync_synchronize();
  while (!peek_host_ready()) ;
  return (uint32_t) read_reg(1);
}

bool simif_zedboard_t::poke_htif_ready() {
  return (uint32_t) read_reg(4) < 32;
}

void simif_zedboard_t::poke_htif(uint32_t value) {
  write_reg(1, value);
  __sync_synchronize();
}

bool simif_zedboard_t::peek_htif_ready() {
  return (uint32_t) read_reg(2) != 0;
}

uint32_t simif_zedboard_t::peek_htif() {
  __sync_synchronize();
  while (!peek_htif_ready()) ;
  return (uint32_t) read_reg(3);
}

void simif_zedboard_t::serve_htif(const size_t size) {
  while (from_htif.size() >= size && poke_htif_ready()) {
    char *buf = new char[size];
    std::copy(from_htif.begin(), from_htif.begin() + size, buf);
    uint32_t *value = (uint32_t*) buf;
    poke_htif(*value);
    from_htif.erase(from_htif.begin(), from_htif.begin() + size);
  }
  while (peek_htif_ready()) {
    uint32_t value = peek_htif();
    to_htif.insert(to_htif.end(), (const char*)&value, (const char*)&value + size);
  }
}

int simif_zedboard_t::run() {
  load_mem();
  return simif_t::run();
}
