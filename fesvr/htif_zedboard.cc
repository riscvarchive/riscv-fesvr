// See LICENSE for license details.

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
  write_reg(31, 0); // reset

  mem = (uintptr_t*)mmap(0, mem_mb() << 20, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  assert(mem != MAP_FAILED);
}

htif_zedboard_t::~htif_zedboard_t()
{
}

ssize_t htif_zedboard_t::write(const void* buf, size_t size)
{
  poll_mem();

  const uint16_t* x = (const uint16_t*)buf;
  assert(size >= sizeof(*x));
  write_reg(0, *x);
  return sizeof(*x);
}

ssize_t htif_zedboard_t::read(void* buf, size_t max_size)
{
  poll_mem();

  uint16_t* x = (uint16_t*)buf;
  assert(max_size >= sizeof(*x));
  uintptr_t v = read_reg(0);
  *x = v >> 1;
  return (v & 1) ? sizeof(*x) : 0;
}

void htif_zedboard_t::poll_mem()
{
  uintptr_t cmd = read_reg(1);
  if (cmd & 1)
  {
    bool read = (cmd & 2) == 0;
    uintptr_t addr = (cmd >> 2)*chunk_align();
    assert(addr < (mem_mb() << 20));

    for (size_t i = 0; i < chunk_align()/sizeof(uintptr_t); i++)
    {
      uintptr_t* x = mem + addr/sizeof(uintptr_t) + i;
      if (read)
        write_reg(1, *x);
      else
        *x = read_reg(2);
    }
  }
}
