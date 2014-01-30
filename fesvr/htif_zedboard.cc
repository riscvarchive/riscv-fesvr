#include "htif_zedboard.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>

#define read_reg(r) (dev_vaddr[r])
#define write_reg(r, v) (dev_vaddr[r] = v)

htif_zedboard_t::htif_zedboard_t(const std::vector<std::string>& args)
  : htif_t(args)
{
  int fd = open("/dev/mem", O_RDWR|O_SYNC);
  assert(fd != -1);
  dev_vaddr = (uintptr_t*)mmap(0, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, dev_paddr);
  assert(dev_vaddr != MAP_FAILED);

  write_reg(31, 0); // pulses reset_sync signal to EOS18 chip
}

htif_zedboard_t::~htif_zedboard_t()
{
}

ssize_t htif_zedboard_t::write(const void* buf, size_t size)
{
  const uint16_t* x = (const uint16_t*)buf;
  assert(size >= sizeof(*x));
  write_reg(0, *x);
  return sizeof(*x);
}

ssize_t htif_zedboard_t::read(void* buf, size_t max_size)
{
  uint16_t* x = (uint16_t*)buf;
  assert(max_size >= sizeof(*x));
  uintptr_t v = read_reg(0);
  *x = v >> 1;
  return (v & 1) ? sizeof(*x) : 0;
}

