#ifndef __HTIF_RS232_H
#define __HTIF_RS232_H

#include "interface.h"
#include "htif.h"

const size_t RS232_DATA_ALIGN = 8;
const size_t RS232_MAX_DATA_SIZE = 8;

class htif_rs232_t : public htif_t
{
public:
  htif_rs232_t(const char* tty);
  virtual ~htif_rs232_t();
  void read_packet(packet_t* p, int expected_seqno);
  void write_packet(const packet_t* p);

  void start();
  void stop();
  void read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd=IF_MEM);
  void write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd=IF_MEM);
  reg_t read_cr(int coreid, int regnum);
  void write_cr(int coreid, int regnum, reg_t val);
  size_t chunk_align();

protected:
  int fd;
  uint16_t seqno;
};

#endif // __HTIF_RS232_H
