// See LICENSE for license details.

#ifndef __HTIF_PACKET_H
#define __HTIF_PACKET_H

#include <stdexcept>
#include <stdint.h>
#include <string>
#include <assert.h>
#include <cstring>

enum
{
  HTIF_CMD_READ_MEM,
  HTIF_CMD_WRITE_MEM,
  HTIF_CMD_READ_CONTROL_REG,
  HTIF_CMD_WRITE_CONTROL_REG,
  HTIF_CMD_ACK,
  HTIF_CMD_NACK,
};

#define HTIF_DATA_ALIGN 8U

typedef uint8_t seqno_t;
typedef uint64_t reg_t;
typedef int64_t sreg_t;
typedef reg_t addr_t;

struct packet_header_t
{
  reg_t cmd       :  4;
  reg_t data_size : 12;
  reg_t seqno     :  8;
  reg_t addr      : 40;

  packet_header_t()
    : cmd(0), data_size(0), seqno(0), addr(0) {}
  packet_header_t(reg_t c, seqno_t s, reg_t ds, addr_t a)
    : cmd(c), data_size(ds), seqno(s), addr(a) {}
  packet_header_t(const void* buf)
  {
    memcpy(this, buf, sizeof(*this));
  }
  size_t get_payload_size() const
  {
    if (cmd == HTIF_CMD_READ_MEM || cmd == HTIF_CMD_READ_CONTROL_REG)
      return 0;
    return data_size * HTIF_DATA_ALIGN;
  }
  size_t get_packet_size() const { return sizeof(*this) + get_payload_size(); }
};

class packet_t
{
 public:
  packet_t(const packet_header_t& hdr);
  packet_t(const packet_header_t& hdr, const void* payload, size_t paysize);
  packet_t(const void* packet);
  packet_t(const packet_t& p);
  ~packet_t();

  packet_header_t get_header() const { return header; }
  const uint8_t* get_payload() const { return packet + sizeof(header); }
  size_t get_size() const { return header.get_packet_size(); }
  const uint8_t* get_packet() const { return packet; }
  size_t get_payload_size() const { return header.get_payload_size(); }

 private:
  packet_header_t header;
  uint8_t* packet;

  void init(const void* payload, size_t payload_size);
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

#endif
