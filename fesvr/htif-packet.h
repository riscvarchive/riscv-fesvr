#ifndef __HTIF_PACKET_H
#define __HTIF_PACKET_H

#include <stdexcept>
#include <stdint.h>
#include <assert.h>

enum
{
  HTIF_CMD_READ_MEM,
  HTIF_CMD_WRITE_MEM,
  HTIF_CMD_READ_CONTROL_REG,
  HTIF_CMD_WRITE_CONTROL_REG,
  HTIF_CMD_ACK,
  HTIF_CMD_NACK,
};

#define HTIF_DATA_ALIGN 8

typedef uint8_t seqno_t;
typedef uint64_t reg_t;
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
    : cmd(c), data_size(ds), seqno(s), addr(a)
  {
    assert(cmd == c && data_size == ds && seqno == s && addr == a);
  }
};

class packet_t
{
 public:
  packet_t(const packet_header_t& hdr);
  packet_t(const packet_t& p);
  ~packet_t();

  void set_payload(const void* payload, size_t size);

  packet_header_t get_header() const { return header; }
  const uint8_t* get_payload() const { return payload; }
  size_t get_payload_size() const { return payload_size; }
  size_t get_size() const;

 private:
  packet_header_t header;
  uint8_t* payload;
  size_t payload_size;
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
