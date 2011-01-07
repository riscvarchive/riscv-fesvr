#ifndef __HTIF_RS232_H
#define __HTIF_RS232_H

#include "interface.h"
#include "htif.h"

enum
{
  RS232_CMD_READ_MEM,
  RS232_CMD_WRITE_MEM,
  RS232_CMD_READ_CONTROL_REG,
  RS232_CMD_WRITE_CONTROL_REG,
  RS232_CMD_START,
  RS232_CMD_STOP,
  RS232_CMD_ACK,
  RS232_CMD_NACK,
};

const size_t RS232_DATA_ALIGN = 8;
const size_t RS232_MAX_DATA_SIZE = 8;

struct rs232_packet_t
{
  uint16_t cmd;
  uint16_t seqno;
  uint32_t data_size;
  uint64_t addr;
  uint8_t  data[RS232_MAX_DATA_SIZE];
};

class htif_rs232_t : public htif_t
{
public:
  htif_rs232_t(const char* tty);
  virtual ~htif_rs232_t();
  void read_packet(rs232_packet_t* p, int expected_seqno);
  void write_packet(const rs232_packet_t* p);

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

class rs232_packet_error : public std::runtime_error
{
public:
  rs232_packet_error(const std::string& s) : std::runtime_error(s) {}
};
class rs232_bad_seqno_error : public rs232_packet_error
{
public:
  rs232_bad_seqno_error() : rs232_packet_error("bad seqno") {}
};
class rs232_io_error : public rs232_packet_error
{
public:
  rs232_io_error(const std::string& s) : rs232_packet_error(s) {}
};

#endif // __HTIF_RS232_H
