#ifndef __HTIF_CSIM_H
#define __HTIF_CSIM_H

#include "interface.h"
#include "htif.h"

const size_t C_DATA_ALIGN = 8;
const size_t C_MAX_DATA_SIZE = 8;

class htif_csim_t : public htif_t
{
public:
  htif_csim_t(int _fdin, int _fdout);
  virtual ~htif_csim_t();
  void read_packet(packet_t* p, int expected_seqno);
  void write_packet(const packet_t* p);

  void start(int coreid);
  void stop(int coreid);
  void read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd=IF_MEM);
  void write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd=IF_MEM);
  reg_t read_cr(int coreid, int regnum);
  void write_cr(int coreid, int regnum, reg_t val);
  size_t chunk_align();

protected:
  int fdin;
  int fdout;
  uint16_t seqno;
};

#endif // __HTIF_CSIM_H
