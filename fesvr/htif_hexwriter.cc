#include <iostream>
#include "htif_hexwriter.h"

void htif_hexwriter_t::read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd)
{
  demand(cmd == IF_MEM && taddr % chunk_align() == 0, "taddr=%016lx read_chunk misaligned", taddr);
  demand(len % chunk_align() == 0, "len=%ld read_chunk misaligned", len);

  while(len)
  {
    if(mem[taddr/width].size() == 0)
      mem[taddr/width].resize(width,0);

    for(size_t j = 0; j < width; j++)
      dst[j] = mem[taddr/width][j];

    len -= width;
    taddr += width;
    dst += width;
  }
}

void htif_hexwriter_t::write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd)
{
  demand(cmd == IF_MEM && taddr % chunk_align() == 0, "taddr=%016lx read_chunk misaligned", taddr);
  demand(len % chunk_align() == 0, "len=%ld read_chunk misaligned", len);

  while(len)
  {
    if(mem[taddr/width].size() == 0)
      mem[taddr/width].resize(width,0);

    for(size_t j = 0; j < width; j++)
      mem[taddr/width][j] = src[j];

    len -= width;
    taddr += width;
    src += width;
  }
}

std::ostream& operator<< (std::ostream& o, const htif_hexwriter_t& h)
{
  std::ios_base::fmtflags flags = o.setf(std::ios::hex,std::ios::basefield);

  std::map<addr_t,std::vector<char> >::const_iterator i;
  for(i = h.mem.begin(); i != h.mem.end(); ++i)
  {
    o << '@' << i->first << ' ';
    for(size_t j = 0; j < h.width; j++)
      o << ((i->second[h.width-j-1] >> 4) & 0xF) << (i->second[h.width-j-1] & 0xF);
    o << std::endl;
  }

  o.setf(flags);

  return o;
}
