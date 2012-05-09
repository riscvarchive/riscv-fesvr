#ifndef __HTIF_HEXWRITER_H
#define __HTIF_HEXWRITER_H

#include <map>
#include <vector>
#include <stdlib.h>
#include "htif.h"

class htif_hexwriter_t : public htif_t
{
public:
  htif_hexwriter_t(size_t w, size_t d) : htif_t(1), width(w), depth(d) {}

protected:
  size_t width;
  size_t depth;
  std::map<addr_t,std::vector<char> > mem;

  void read_chunk(addr_t taddr, size_t len, uint8_t* dst);
  void write_chunk(addr_t taddr, size_t len, const uint8_t* src);

  size_t chunk_max_size() { return width; }
  size_t chunk_align() { return width; }

  ssize_t read(void* buf, size_t max_size) { abort(); }
  ssize_t write(const void* buf, size_t max_size) { abort(); }
  uint32_t mem_mb() { abort(); }

  friend std::ostream& operator<< (std::ostream&, const htif_hexwriter_t&);
};

#endif // __HTIF_HEXWRITER_H
