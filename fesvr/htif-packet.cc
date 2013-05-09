#include "htif-packet.h"
#include <string.h>

packet_t::packet_t(const packet_header_t& hdr, uint64_t a)
  : header(hdr), payload(NULL), addr(a), payload_size(0) 
{
    set_payload(NULL, 0, false);
}

packet_t::packet_t(const packet_t& p)
  : header(p.header), payload(NULL), addr(p.addr), payload_size(0)
{
  set_payload(p.payload, p.payload_size, false);
}

void packet_t::set_payload(const void* pay, size_t size, bool isResp)
{
  delete [] payload;

  int addr_width = 0;
  if (!isResp) {
      addr_width = 1;
      if (header.cmd == HTIF_CMD_READ_MEM || header.cmd == HTIF_CMD_WRITE_MEM) {
          addr_width = 4;
      }
  }

  payload_size = size + addr_width;
  payload = new uint8_t[payload_size];

  if (!isResp) {
      for (int i = 0; i < addr_width; i++)
          payload[i] = (addr >> ((addr_width-i-1)*8)) & ((1 << 8) - 1);
      //memcpy(payload, &addr, addr_width);
  }
    
  if (size) {
    for (int i = 0; i < size; i++)
        payload[addr_width+i] = ((uint8_t *) pay)[size-i-1];
    //memcpy(payload+addr_width, pay, payload_size-addr_width);
     if (size == 64) {
        uint8_t *temp = new uint8_t[64];
        // 0,1,2,3 -> 3,2,1,0
        for (int i = 0; i < 4; i++)
            memcpy(temp+16*i, payload+addr_width+i*16, 16);
        for (int i = 0; i < 4; i++)
            memcpy(payload+addr_width+i*16, temp + 16*(3-i), 16);
    }
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
