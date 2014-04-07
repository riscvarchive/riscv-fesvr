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

  printf("about to pulse reset signal\n");
  write_reg(31, 1);
  write_reg(31, 0);
  printf("reset completed\n");
}

htif_zedboard_t::~htif_zedboard_t()
{
}

float htif_zedboard_t::get_host_clk_freq()
{
  uintptr_t f1, f2;
  // read multiple times to ensure accuracy
  do {
    f1 = read_reg(2);
    usleep(10000);
    f2 = read_reg(2);
  } while (abs(f1-f2) > 500);

  float freq = f1 / 1E4;
  return freq;
}

ssize_t htif_zedboard_t::write(const void* buf, size_t size)
{
  const uint32_t* x = (const uint32_t*)buf;
  assert(size >= sizeof(*x));
  write_reg(0, *x);
  return sizeof(*x);
}

ssize_t htif_zedboard_t::read(void* buf, size_t max_size)
{
  uint32_t* x = (uint32_t*)buf;
  assert(max_size >= sizeof(*x));

  // fifo data counter
  uintptr_t c = read_reg(1); 
  uint32_t count = 0;
  if (c > 0)
    for (count=0; count<c && count*sizeof(*x)<max_size; count++)
      x[count] = read_reg(0);

  return count*sizeof(*x);
}
