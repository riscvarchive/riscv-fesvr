// See LICENSE for license details.

#ifndef __HTIF_ETH_H
#define __HTIF_ETH_H

#include "htif.h"
#include <vector>

#include <sys/socket.h>
#ifdef __linux__
#include <netpacket/packet.h>
typedef struct sockaddr_ll sockaddr_ll_t;
#elif __APPLE__ || __FreeBSD__
#include <sys/types.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#if __APPLE__
#include <net/ndrv.h>
#endif
typedef struct sockaddr sockaddr_ll_t;
#else
# error unsupported OS
#endif

const size_t ETH_DATA_ALIGN = 64;
const size_t ETH_MAX_DATA_SIZE = 1472; // largest multiple of 64 <= MTU
const unsigned short HTIF_ETHERTYPE = 0x8888;

class htif_eth_t : public htif_t
{
public:
  htif_eth_t(const std::vector<std::string>& args);
  virtual ~htif_eth_t();

  size_t chunk_align() { return ETH_DATA_ALIGN; }
  size_t chunk_max_size() { return ETH_MAX_DATA_SIZE; }

protected:
  ssize_t read(void* buf, size_t max_size);
  ssize_t write(const void* buf, size_t size);

  int sock;
  sockaddr_ll_t src_addr;
  char src_mac[6];
  char dst_mac[6];

  bool rtlsim;

  int buffer_len;
};

#endif // __HTIF_ETH_H
