#ifndef __HTIF_ISASIM_H
#define __HTIF_ISASIM_H

#include "interface.h"
#include "htif.h"

enum
{
  APP_CMD_READ_MEM,
  APP_CMD_WRITE_MEM,
  APP_CMD_READ_CONTROL_REG,
  APP_CMD_WRITE_CONTROL_REG,
  APP_CMD_START,
  APP_CMD_STOP,
  APP_CMD_ACK,
  APP_CMD_NACK,
};

const size_t APP_DATA_ALIGN = 8;
const size_t APP_MAX_DATA_SIZE = 1024;

struct packet_t
{
  uint16_t cmd;
  uint16_t seqno;
  uint32_t data_size;
  uint64_t addr;
  uint8_t  data[APP_MAX_DATA_SIZE];
};

class htif_isasim_t : public htif_t
{
public:
  htif_isasim_t(int _fdin, int _fdout);
  virtual ~htif_isasim_t();
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
  int fdin;
  int fdout;
  uint16_t seqno;
};

class packet_error : public std::runtime_error
{
public:
  packet_error(const std::string& s) : std::runtime_error(s) {}
};
class bad_seqno_error : public packet_error
{
public:
  bad_seqno_error() : packet_error("bad seqno") {}
};
class io_error : public packet_error
{
public:
  io_error(const std::string& s) : packet_error(s) {}
};

#endif // __HTIF_ISASIM_H
