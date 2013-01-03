#ifndef __HTIF_ISASIM_H
#define __HTIF_ISASIM_H

#include "htif.h"
#include <unistd.h>
#include <vector>

class htif_isasim_t : public htif_t
{
 public:
  htif_isasim_t(std::vector<char*> args);

 protected:
  ssize_t read(void* buf, size_t max_size)
  {
    return ::read(fdin, buf, max_size);
  }

  ssize_t write(const void* buf, size_t size, bool hack_reset)
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
