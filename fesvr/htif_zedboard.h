#ifndef __HTIF_ZEDBOARD_H
#define __HTIF_ZEDBOARD_H

#include "htif.h"
#include <vector>

class htif_zedboard_t : public htif_t
{
 public:
  htif_zedboard_t(int ncores, std::vector<char*> args);
  ~htif_zedboard_t();

  uint32_t mem_mb() { return 256; }

 protected:
  ssize_t read(void* buf, size_t max_size);
  ssize_t write(const void* buf, size_t size);

  size_t chunk_max_size() { return 64; }
  size_t chunk_align() { return 64; }

 private:
  void poll_mem();
  unsigned memsize_mb;
  uintptr_t* mem;
  volatile uintptr_t* dev_vaddr;

  const static uintptr_t dev_paddr = 0x40000000;
};

#endif // __HTIF_ZEDBOARD_H
