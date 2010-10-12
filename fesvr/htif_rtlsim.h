#ifndef __HTIF_RTLSIM_H
#define __HTIF_RTLSIM_H

#include "interface.h"
#include "htif.h"

enum
{
  RTL_CMD_READ_MEM,
  RTL_CMD_WRITE_MEM,
  RTL_CMD_READ_CONTROL_REG,
  RTL_CMD_WRITE_CONTROL_REG,
  RTL_CMD_START,
  RTL_CMD_STOP,
  RTL_CMD_ACK,
  RTL_CMD_NACK,
};

const size_t RTL_DATA_ALIGN = 8;
const size_t RTL_MAX_DATA_SIZE = 8;

struct rtl_packet_t
{
  uint16_t cmd;
  uint16_t seqno;
  uint32_t data_size;
  uint64_t addr;
  uint8_t  data[RTL_MAX_DATA_SIZE];
};

class htif_rtlsim_t : public htif_t
{
public:
  htif_rtlsim_t(int _fdin, int _fdout);
  virtual ~htif_rtlsim_t();
  void read_packet(rtl_packet_t* p, int expected_seqno);
  void write_packet(const rtl_packet_t* p);

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

class rtl_packet_error : public std::runtime_error
{
public:
  rtl_packet_error(const std::string& s) : std::runtime_error(s) {}
};
class rtl_bad_seqno_error : public rtl_packet_error
{
public:
  rtl_bad_seqno_error() : rtl_packet_error("bad seqno") {}
};
class rtl_io_error : public rtl_packet_error
{
public:
  rtl_io_error(const std::string& s) : rtl_packet_error(s) {}
};

#endif // __HTIF_RTLSIM_H
