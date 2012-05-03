#ifndef __HTIF_ETH_H
#define __HTIF_ETH_H

#include "htif.h"
#include <vector>

#include <sys/socket.h>
#ifdef __linux__
#include <netpacket/packet.h>
typedef struct sockaddr_ll sockaddr_ll_t;
#elif __APPLE__
#include <sys/types.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/ndrv.h>
typedef struct sockaddr sockaddr_ll_t;
#else
# error unsupported OS
#endif

const size_t ETH_DATA_ALIGN = 64;
const size_t ETH_MAX_DATA_SIZE = 64;
const unsigned short HTIF_ETHERTYPE = 0x8888;

class htif_eth_t : public htif_t
{
public:
  htif_eth_t(std::vector<char*> args);
  virtual ~htif_eth_t();

  void start(int coreid)
  {
    // write memory size (in MB) and # cores in words 0, 1
    uint32_t buf[16] = {512,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    write_chunk(0, sizeof(buf), (uint8_t *)buf);
  
    htif_t::start(coreid);
  }

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
