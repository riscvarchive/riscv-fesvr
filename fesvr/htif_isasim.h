#ifndef __HTIF_ISASIM_H
#define __HTIF_ISASIM_H

#include "htif.h"
#include <unistd.h>

class htif_isasim_t : public htif_t
{
 public:
  htif_isasim_t(int _fdin, int _fdout)
    : fdin(_fdin), fdout(_fdout)
  {
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

#endif // __HTIF_ISASIM_H
