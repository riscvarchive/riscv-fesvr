#ifndef __HTIF_CSIM_H
#define __HTIF_CSIM_H

#include "htif.h"
#include <unistd.h>

class htif_csim_t : public htif_t
{
 public:
  htif_csim_t(int _fdin, int _fdout)
    : fdin(_fdin), fdout(_fdout)
  {
  }

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
  uint16_t seqno;
};

typedef htif_csim_t htif_rtlsim_t;

#endif // __HTIF_CSIM_H
