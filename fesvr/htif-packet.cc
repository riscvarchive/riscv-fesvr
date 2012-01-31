#include "htif-packet.h"
#include <string.h>

packet_t::packet_t(const packet_header_t& hdr)
  : header(hdr), payload(NULL), payload_size(0)
{
}

packet_t::packet_t(const packet_t& p)
  : header(p.header), payload(NULL), payload_size(0)
{
  set_payload(p.payload, p.payload_size);
}

void packet_t::set_payload(const void* pay, size_t size)
{
  delete [] payload;

  payload = NULL;
  payload_size = size;

  if (payload_size)
  {
    payload = new uint8_t[payload_size];
    memcpy(payload, pay, payload_size);
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
