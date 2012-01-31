#include "htif_eth.h"
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

#define debug(...)
//#define debug(...) fprintf(stderr,__VA_ARGS__)

struct eth_packet_t
{
  char dst_mac[6];
  char src_mac[6];
  unsigned short ethertype;
  short pad;
  packet_header_t htif_header;
  char htif_payload[ETH_MAX_DATA_SIZE];
};

htif_eth_t::htif_eth_t(std::vector<char*> args)
  : rtlsim(false)
{
  const char* interface = NULL;
  for (size_t i = 0; i < args.size(); i++)
  {
    if (strncmp(args[i], "+if=", 4) == 0)
      interface = args[i] + 4;
    if (strcmp(args[i], "+sim") == 0)
      rtlsim = true;
  }

#ifndef __linux__
  assert(0); // TODO: add OSX support
#else
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
    // setuid root to open a raw socket.  if we fail, too bad
    seteuid(0);
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    seteuid(getuid());
    if(sock < 0)
      throw std::runtime_error("socket() failed!");

    // get MAC address of local ethernet device
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface);
    if(ioctl(sock, SIOCGIFHWADDR, (char*)&ifr) == -1)
      throw std::runtime_error("ioctl() failed!");
    memcpy(&src_mac, &ifr.ifr_ifru.ifru_hwaddr.sa_data, sizeof(src_mac));

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
#endif
}

htif_eth_t::~htif_eth_t()
{
  close(sock);
}

ssize_t htif_eth_t::read(void* buf, size_t max_size)
{
  ssize_t bytes;

  for(int timeouts = 0; timeouts < 3; )
  {
    eth_packet_t packet;

    if((bytes = ::read(sock, (char*)&packet, sizeof(packet))) == -1)
    {
      timeouts++;
      debug("read failed (%s)\n",strerror(errno));
      continue;
    }

    debug("read packet\n");
    for(ssize_t i = 0; i < bytes; i++)
      debug("%x ",((unsigned char*)&packet)[i]);
    debug("\n");

    if(packet.ethertype != htons(HTIF_ETHERTYPE))
    {
      debug("wrong ethertype\n");
      continue;
    }

    bytes -= 16; //offsetof(eth_header_t, htif_header)
    memcpy(buf, &packet.htif_header, bytes);
    return bytes;
  }

  return -1;
}

ssize_t htif_eth_t::write(const void* buf, size_t size)
{
  debug("write packet\n");
  for(size_t i = 0; i < size; i++)
    debug("%x ",((const unsigned char*)buf)[i]);
  debug("\n");

  eth_packet_t eth_packet;
  memset(eth_packet.dst_mac, -1, sizeof(src_mac));
  memcpy(eth_packet.src_mac, src_mac, sizeof(src_mac));
  eth_packet.ethertype = htons(HTIF_ETHERTYPE);
  eth_packet.pad = 0;
  memcpy(&eth_packet.htif_header, buf, size);
  size += 16; //offsetof(eth_packet_t, htif_header)

  if(rtlsim)
    return ::write(sock, (char*)&eth_packet, size);
  else
    return ::sendto(sock, (char*)&eth_packet, size, 0,
                    (sockaddr*)&src_addr, sizeof(src_addr));
}
