#ifndef __HTIF_CSIM_H
#define __HTIF_CSIM_H

#include "htif.h"
#include <vector>

class htif_csim_t : public htif_t
{
 public:
  htif_csim_t(int ncores, const char* progname, std::vector<char*> args);
  ~htif_csim_t();

  uint32_t mem_mb() { return 512; }

 protected:
  ssize_t read(void* buf, size_t max_size)
  {
    return ::read(fdin, buf, max_size);
  }

  ssize_t write(const void* buf, size_t size)
  {
    return ::write(fdout, buf, size);
  }

  size_t chunk_max_size() { return 2048; }
  size_t chunk_align() { return 64; }

 private:
  int pid;
  int fdin;
  int fdout;
};

#endif // __HTIF_CSIM_H
