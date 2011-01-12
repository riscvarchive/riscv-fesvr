#ifndef __HTIF_ETH_H
#define __HTIF_ETH_H

#include <sys/socket.h>
#include <netpacket/packet.h>
#include "interface.h"
#include "htif.h"

const size_t ETH_DATA_ALIGN = 8;
const size_t ETH_MAX_DATA_SIZE = 8;

const unsigned short HTIF_ETHERTYPE = 0x8888;

class htif_eth_t : public htif_t
{
public:
  htif_eth_t(const char* interface, bool rtlsim);
  virtual ~htif_eth_t();
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
  int sock;
  struct sockaddr_ll src_addr;
  char src_mac[6];
  char dst_mac[6];

  uint16_t seqno;
  bool rtlsim;
};

#endif // __HTIF_ETH_H
