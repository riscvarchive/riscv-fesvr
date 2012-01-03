#ifndef __HTIF_ETH_H
#define __HTIF_ETH_H

#include <sys/socket.h>
#include "interface.h"
#include "htif.h"
const size_t ETH_REG_ALIGN = 8;
const size_t ETH_DATA_ALIGN = 64;
const size_t ETH_MAX_DATA_SIZE = 64;

#ifdef __linux__
#include <netpacket/packet.h>
typedef struct sockaddr_ll sockaddr_ll_t;
#else
#include <sys/types.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/ndrv.h>
typedef struct sockaddr sockaddr_ll_t;
#endif

const unsigned short HTIF_ETHERTYPE = 0x8888;

class htif_eth_t : public htif_t
{
public:
  htif_eth_t(const char* interface, bool rtlsim);
  virtual ~htif_eth_t();
  void read_packet(packet_t* p, int expected_seqno);
  void write_packet(const packet_t* p);

  void start(int coreid);
  void stop(int coreid);
  void read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd=IF_MEM);
  void write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd=IF_MEM);
  reg_t read_cr(int coreid, int regnum);
  void write_cr(int coreid, int regnum, reg_t val);
  size_t chunk_align();

protected:
  int sock;
  sockaddr_ll_t src_addr;
  char src_mac[6];
  char dst_mac[6];

  uint16_t seqno;
  bool rtlsim;
};

#endif // __HTIF_ETH_H
