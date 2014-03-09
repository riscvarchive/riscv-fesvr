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
}

htif_zedboard_t::~htif_zedboard_t()
{
}

ssize_t htif_zedboard_t::write(const void* buf, size_t size)
{
//  printf("htif_zedboard_t::write(buf, size = %u\n", size);
  const uint32_t* x = (const uint32_t*)buf;
  assert(size >= sizeof(*x));

  for (uint32_t i = 0; i < size/sizeof(*x); i++)
    write_reg(0, x[i]);
//  printf("htif_zedboard_t::write() returning\n");
//  return sizeof(*x);
  return size;
}

ssize_t htif_zedboard_t::read(void* buf, size_t max_size)
{
//  printf("htif_zedboard_t::read(buf, max_size = %u\n", max_size);
  uint32_t* x = (uint32_t*)buf;
  assert(max_size >= sizeof(*x));

  // fifo data counter
  uintptr_t c = read_reg(0); 
  uint32_t count = 0;
  if (c > 0)
  {
//    printf("htif_zedboard_t::read() read_reg(0) returned %u\n", c);
    for (count=0; count<c && count*sizeof(*x)<max_size; count++)
      x[count] = read_reg(1);
  }
//  printf("htif_zedboard_t::read() returning %u\n", count*sizeof(*x));
  return count*sizeof(*x);
}

