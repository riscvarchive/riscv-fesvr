#ifndef __HTIF_RTLSIM_H
#define __HTIF_RTLSIM_H

#include "htif.h"
#include <unistd.h>
#include <vector>

class htif_rtlsim_t : public htif_t
{
 public:
  htif_rtlsim_t(std::vector<char*> args);

  void start(int coreid)
  {
    // write memory size (in MB) and # cores in words 0, 1
    uint32_t buf[4] = {512,1,0,0};
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

  size_t chunk_max_size() { return 1024; }
  size_t chunk_align() { return 16; }

 private:
  int fdin;
  int fdout;
};

#endif // __HTIF_RTLSIM_H
