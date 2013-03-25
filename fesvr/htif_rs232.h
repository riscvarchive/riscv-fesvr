// See LICENSE for license details.

#ifndef __HTIF_RS232_H
#define __HTIF_RS232_H

#include "htif.h"
#include <vector>

class htif_rs232_t : public htif_t
{
 public:
  htif_rs232_t(const std::vector<std::string>& args);
  ~htif_rs232_t();

 protected:
  ssize_t read(void* buf, size_t max_size);
  ssize_t write(const void* buf, size_t size);

  size_t chunk_max_size() { return 64; }
  size_t chunk_align() { return 64; }

 private:
  int fd;
};

#endif // __HTIF_RS232_H
