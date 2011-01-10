#include <algorithm>
#include <sys/types.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <net/ethernet.h>
#include <net/if.h>
#include "common.h"
#include "interface.h"
#include "htif_eth.h"
#include "memif.h"

#define HTIF_MAX_DATA_SIZE ETH_MAX_DATA_SIZE
#include "htif-packet.h"

//#define debug(...)
#define debug(...) fprintf(stderr,__VA_ARGS__)

struct eth_packet_t
{
  char dst_mac[6];
  char src_mac[6];
  short ethertype;
  short pad;
  packet_t htif_packet;
};

htif_eth_t::htif_eth_t(const char* interface, bool _rtlsim)
: seqno(1), rtlsim(_rtlsim)
{
  if(rtlsim)
  {
    const char* socket_path = interface;
    char socket_path_client[strlen(socket_path)+8];
    strcpy(socket_path_client, socket_path);
    strcat(socket_path_client, "client");

    struct sockaddr_un vs_addr, appserver_addr;

    // unlink should fail if nothing is wrong.
    if (unlink(socket_path_client) == 0)
      printf("Warning: removing existing unix domain socket %s\n", socket_path_client);

    // Create the socket.
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (sock < 0)
      throw std::runtime_error("socket() failed: client unix domain socket");

    //setup socket address
    memset(&vs_addr, 0, sizeof(vs_addr));
    memset(&appserver_addr, 0, sizeof(appserver_addr));

    vs_addr.sun_family = AF_UNIX;
    strcpy(vs_addr.sun_path, socket_path);

    appserver_addr.sun_family = AF_UNIX;
    strcpy(appserver_addr.sun_path, socket_path_client);

    //bind to client address
    if (bind(sock, (struct sockaddr*)&appserver_addr, sizeof(appserver_addr))<0)
    {
      perror("bind appserver");
      abort();
    }

    if (connect(sock, (struct sockaddr *)&vs_addr, sizeof(vs_addr)) < 0)
      throw std::runtime_error("connect() failed: client unix domain socket");
  }
  else
  {
    // get MAC address of local ethernet device
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface);
    if(ioctl(sock, SIOCGIFHWADDR, (char*)&ifr) == -1)
      throw std::runtime_error("ioctl() failed!");
    memcpy(&src_mac, &ifr.ifr_ifru.ifru_hwaddr.sa_data, sizeof(src_mac));

    // setuid root to open a raw socket.  if we fail, too bad
    seteuid(0);
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    seteuid(getuid());
    if(sock < 0)
      throw std::runtime_error("socket() failed!");

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sll_ifindex = if_nametoindex(interface);
    src_addr.sll_family = AF_PACKET;

    if(bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr)) == -1)
      throw std::runtime_error("bind() failed!");

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(struct timeval)) == -1)
      throw std::runtime_error("setsockopt() failed!");
  }
}

htif_eth_t::~htif_eth_t()
{
}

void htif_eth_t::read_packet(packet_t* p, int expected_seqno)
{
  int bytes;
  while(true)
  {
    eth_packet_t packet;

    if((bytes = read(sock, (char*)&packet, sizeof(packet))) == -1)
    {
      debug("read failed (%s)\n",strerror(errno));
      continue;
    }

    if(packet.ethertype != htons(HTIF_ETHERTYPE))
    {
      debug("wrong ethertype\n");
      continue;
    }

    *p = packet.htif_packet;
  }

  debug("read packet\n");
  for(int i = 0; i < bytes; i++)
    debug("%x ",((unsigned char*)p)[i]);
  debug("\n");

  if (bytes < (int)offsetof(packet_t, data))
    throw io_error("read failed");
  if (p->seqno != expected_seqno)
    throw bad_seqno_error(); 
  switch (p->cmd)
  {
    case HTIF_CMD_ACK:
      if (p->data_size != bytes - offsetof(packet_t, data))
        throw packet_error("bad payload size!");
      break;
    case HTIF_CMD_NACK:
      throw packet_error("nack!");
    default:
      throw packet_error("illegal command");
  }
}

void htif_eth_t::write_packet(const packet_t* p)
{
  int size = offsetof(packet_t, data);
  if(p->cmd == HTIF_CMD_WRITE_MEM || p->cmd == HTIF_CMD_WRITE_CONTROL_REG)
    size += p->data_size;
    
  debug("write packet\n");
  for(int i = 0; i < size; i++)
    debug("%x ",((const unsigned char*)p)[i]);
  debug("\n");

  eth_packet_t eth_packet;
  memset(eth_packet.dst_mac, -1, sizeof(src_mac));
  memcpy(eth_packet.src_mac, src_mac, sizeof(src_mac));
  eth_packet.ethertype = htons(HTIF_ETHERTYPE);
  eth_packet.pad = 0;
  eth_packet.htif_packet = *p;

  size += offsetof(eth_packet_t, htif_packet);

  int bytes;
  if(rtlsim)
    bytes = write(sock, (char*)&eth_packet, size);
  else
    bytes = sendto(sock, (char*)&eth_packet, size, 0,
                   (sockaddr*)&src_addr, sizeof(src_addr));

  if (bytes != size)
    throw io_error("write failed");
}

void htif_eth_t::start()
{
  packet_t p = {HTIF_CMD_START, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_eth_t::stop()
{
  packet_t p = {HTIF_CMD_STOP, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_eth_t::read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd)
{
  demand(cmd == IF_CREG || taddr % chunk_align() == 0, "taddr=%016lx read_chunk misaligned", taddr);
  demand(len % chunk_align() == 0, "len=%ld read_chunk misaligned", len);

  packet_t req;
  packet_t resp;

  if (cmd == IF_MEM)
    req.cmd = HTIF_CMD_READ_MEM;
  else if (cmd == IF_CREG)
    req.cmd = HTIF_CMD_READ_CONTROL_REG;
  else
    demand(0, "unreachable");

  while (len)
  {
    size_t sz = std::min(len, ETH_MAX_DATA_SIZE);

    req.seqno = seqno;
    req.data_size = sz;
    req.addr = taddr;

    write_packet(&req);
    read_packet(&resp, seqno);
    seqno++;

    memcpy(dst, resp.data, sz);

    len -= sz;
    taddr += sz;
    dst += sz;
  }
}

void htif_eth_t::write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd)
{
  demand(cmd == IF_CREG || taddr % chunk_align() == 0, "taddr=%016lx write_chunk misaligned", taddr);
  demand(len % chunk_align() == 0, "len=%ld write_chunk misaligned", len);

  packet_t req;
  packet_t resp;

  if (cmd == IF_MEM)
    req.cmd = HTIF_CMD_WRITE_MEM;
  else if (cmd == IF_CREG)
    req.cmd = HTIF_CMD_WRITE_CONTROL_REG;
  else
    demand(0, "unreachable");

  while (len)
  {
    size_t sz = std::min(len, ETH_MAX_DATA_SIZE);

    req.seqno = seqno;
    req.data_size = sz;
    req.addr = taddr;

    memcpy(req.data, src, sz);

    write_packet(&req);
    read_packet(&resp, seqno);
    seqno++;

    len -= sz;
    taddr += sz;
    src += sz;
  }
}

reg_t htif_eth_t::read_cr(int coreid, int regnum)
{
  reg_t val;
  do
  {
    read_chunk((addr_t)coreid<<32|regnum, sizeof(reg_t), (uint8_t*)&val, IF_CREG);
  } while (val == 0);
  return val;
}

void htif_eth_t::write_cr(int coreid, int regnum, reg_t val)
{
  write_chunk((addr_t)coreid<<32|regnum, sizeof(reg_t), (uint8_t*)&val, IF_CREG);
}

size_t htif_eth_t::chunk_align()
{
  return ETH_DATA_ALIGN;
}
