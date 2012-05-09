#ifndef __HTIF_ISASIM_H
#define __HTIF_ISASIM_H

#include "htif.h"
#include <vector>

class htif_isasim_t : public htif_t
{
 public:
  htif_isasim_t(int ncores, std::vector<char*> args);
  ~htif_isasim_t();

  uint32_t mem_mb() { return memsize_mb; }

 protected:
  ssize_t read(void* buf, size_t max_size)
  {
    return ::read(fdin, buf, max_size);
  }

  ssize_t write(const void* buf, size_t size)
  {
    return ::write(fdout, buf, size);
  }

  size_t chunk_max_size() { return 1024; }
  size_t chunk_align() { return 16; }

 private:
  uint32_t memsize_mb;
  int pid;
  int fdin;
  int fdout;
};

#endif // __HTIF_ISASIM_H
