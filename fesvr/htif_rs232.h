#ifndef __HTIF_RS232_H
#define __HTIF_RS232_H

#include "htif.h"

class htif_rs232_t : public htif_t
{
 public:
  htif_rs232_t(const char* tty);
  ~htif_rs232_t();

  void start(int coreid)
  {
    // write memory size (in MB) and # cores in words 0, 1
    uint32_t buf[16] = {512,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    write_chunk(0, sizeof(buf), (uint8_t *)buf);
  
    htif_t::start(coreid);
  }

 protected:
  ssize_t read(void* buf, size_t max_size);
  ssize_t write(const void* buf, size_t size);

  size_t chunk_max_size() { return 64; }
  size_t chunk_align() { return 64; }

 private:
  int fd;
};

#endif // __HTIF_RS232_H
