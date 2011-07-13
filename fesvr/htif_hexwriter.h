#ifndef __HTIF_HEXWRITER_H
#define __HTIF_HEXWRITER_H

#include <map>
#include <vector>
#include <stdlib.h>
#include "interface.h"
#include "htif.h"

const size_t MEMORY_WIDTH = 16; // in bytes

class htif_hexwriter_t : public htif_t
{
public:
  htif_hexwriter_t(size_t w, size_t d) : width(w),depth(d) {}
  virtual ~htif_hexwriter_t() {}

  void start(int coreid) { abort(); }
  void stop(int coreid) { abort(); }
  void read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd=IF_MEM);
  void write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd=IF_MEM);
  reg_t read_cr(int coreid, int regnum) { abort(); }
  void write_cr(int coreid, int regnum, reg_t val) { abort(); }
  size_t chunk_align() { return width; }

protected:
  size_t width;
  size_t depth;
  std::map<addr_t,std::vector<char> > mem;

  friend std::ostream& operator<< (std::ostream&, const htif_hexwriter_t&);
};

#endif // __HTIF_HEXWRITER_H
