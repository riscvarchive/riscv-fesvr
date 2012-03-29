#ifndef __HTIF_H
#define __HTIF_H

#include <string.h>
#include "htif-packet.h"

class htif_t
{
 public:
  htif_t();
  virtual ~htif_t();

  virtual void start(int coreid);
  virtual void stop(int coreid);

  virtual reg_t read_cr(int coreid, int regnum);
  virtual void write_cr(int coreid, int regnum, reg_t val);

  virtual void assume0init(bool val = true);

 protected:
  virtual void read_chunk(addr_t taddr, size_t len, uint8_t* dst);
  virtual void write_chunk(addr_t taddr, size_t len, const uint8_t* src);

  virtual size_t chunk_align() = 0;
  virtual size_t chunk_max_size() = 0;

  virtual ssize_t read(void* buf, size_t max_size) = 0;
  virtual ssize_t write(const void* buf, size_t size) = 0;

 private:
  bool writezeros;
  seqno_t seqno;

  virtual packet_t read_packet(seqno_t expected_seqno);
  virtual void write_packet(const packet_t& packet);

  friend class memif_t;
};

#endif // __HTIF_H
