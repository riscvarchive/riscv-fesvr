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
  printf("reset completed\n");
}

htif_zedboard_t::~htif_zedboard_t()
{
}

uintptr_t htif_zedboard_t::get_host_clk_freq()
{
  uintptr_t x = read_reg(2);
  return x;
}

ssize_t htif_zedboard_t::write(const void* buf, size_t size)
{
  const uint32_t* x = (const uint32_t*)buf;
  assert(size >= sizeof(*x));
//  printf("    htif_zedboard_t::write() calling write_reg(0, %08x)\n", *x);
  write_reg(0, *x);
  return sizeof(*x);
}

ssize_t htif_zedboard_t::read(void* buf, size_t max_size)
{
//  printf("htif_zedboard_t::read(buf, max_size = %u)\n", max_size);
  uint32_t* x = (uint32_t*)buf;
  assert(max_size >= sizeof(*x));

  // fifo data counter
  uintptr_t c = read_reg(1); 
  uint32_t count = 0;
  if (c > 0)
  {
//    printf("htif_zedboard_t::read() read_reg(0) returned %u: ", c);
    for (count=0; count<c && count*sizeof(*x)<max_size; count++)
    {
      x[count] = read_reg(0);
//      printf("%08x ", x[count]);
    }
//    printf("\n");
  }
//  printf("htif_zedboard_t::read() returning %u\n", count*sizeof(*x));
  return count*sizeof(*x);
}
