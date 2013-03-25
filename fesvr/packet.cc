// See LICENSE for license details.

#include "packet.h"
#include <string.h>

packet_t::packet_t(const packet_header_t& hdr)
  : header(hdr)
{
  init(NULL, 0);
}

packet_t::packet_t(const void* pkt)
  : header(pkt)
{
  init((char*)pkt + sizeof(header), get_payload_size());
}

packet_t::packet_t(const packet_t& p)
  : header(p.header)
{
  init(p.get_payload(), get_payload_size());
}

packet_t::packet_t(const packet_header_t& hdr, const void* payload, size_t paysize)
  : header(hdr)
{
  init(payload, paysize);
}

void packet_t::init(const void* pay, size_t paysize)
{
  if (paysize != get_payload_size())
    throw packet_error("bad payload paysize");

  if (paysize)
  {
    packet = new uint8_t[sizeof(header) + paysize];
    memcpy(packet, &header, sizeof(header));
    memcpy((char*)packet + sizeof(header), pay, paysize);
  }
  else
    packet = (uint8_t*)&header;
}

packet_t::~packet_t()
{
  if (packet != (uint8_t*)&header)
    delete [] packet;
}
