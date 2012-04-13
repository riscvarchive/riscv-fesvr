#ifndef __HTIF_CSIM_H
#define __HTIF_CSIM_H

#include "htif.h"
#include <vector>

class htif_csim_t : public htif_t
{
 public:
  htif_csim_t(const char* progname, std::vector<char*> args);
  ~htif_csim_t();

  void start(int coreid)
  {
    // write memory size (in MB) and # cores in words 0, 1
    uint32_t buf[16] = {512,1};
    write_chunk(0, sizeof(buf), (uint8_t *)buf);
  
    htif_t::start(coreid);
  }

 protected:
  ssize_t read(void* buf, size_t max_size)
  {
    return ::read(fdin, buf, max_size);
  }

  ssize_t write(const void* buf, size_t size)
  {
    return ::write(fdout, buf, size);
  }

  size_t chunk_max_size() { return 64; }
  size_t chunk_align() { return 64; }

 private:
  int pid;
  int fdin;
  int fdout;
};

#endif // __HTIF_CSIM_H
