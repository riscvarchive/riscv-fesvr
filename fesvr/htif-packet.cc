#include "htif-packet.h"
#include <string.h>

packet_t::packet_t(const packet_header_t& hdr, uint64_t a)
  : header(hdr), payload(NULL), addr(a), payload_size(0) 
{
}

packet_t::packet_t(const packet_t& p)
  : header(p.header), payload(NULL), addr(p.addr), payload_size(0)
{
  set_payload(p.payload, p.payload_size);
}

void packet_t::set_payload(const void* pay, size_t size)
{
  delete [] payload;

  int addr_width = 1;
  if (header.cmd == HTIF_CMD_READ_MEM || header.cmd == HTIF_CMD_WRITE_MEM) {
      addr_width = 5;
  }

  payload_size = size + addr_width;
  payload = new uint8_t[payload_size];
  memcpy(payload, &addr, addr_width);

  if (size) {
    memcpy(payload+addr_width, pay, payload_size-size);
  }
}

packet_t::~packet_t()
{
  delete [] payload;
}

size_t packet_t::get_size() const
{
  return sizeof(packet_header_t) + payload_size;
}
