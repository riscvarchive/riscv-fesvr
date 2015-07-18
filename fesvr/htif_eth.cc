// See LICENSE for license details.

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
#include <ifaddrs.h>

#ifdef __APPLE__
#include <net/bpf.h>
#endif

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

htif_eth_t::htif_eth_t(const std::vector<std::string>& args)
  : htif_t(args), rtlsim(false)
{
  std::string mac_str = "feedfacebeef";
#ifdef __linux__
  std::string interface = "eth0";
#elif __APPLE__
  std::string interface = "en0";
#elif __FreeBSD__
  std::string interface = "em0";
#endif
  for (std::vector<std::string>::const_iterator a = host_args().begin(); a != host_args().end(); ++a)
  {
    if (a->substr(0, 4) == "+if=")
      interface = a->substr(4);
    if (a->substr(0, 5) == "+mac=")
      mac_str = a->substr(5), assert(mac_str.length() == 2*sizeof(dst_mac));
    if (*a == "+sim")
      rtlsim = true;
  }

  #define hexval(c) ((c) >= 'a' ? (c)-'a'+10 : (c) >= 'A' ? (c)-'A'+10 : (c)-'0')
  for (size_t i = 0; i < sizeof(dst_mac); i++)
    dst_mac[i] = hexval(mac_str[2*i]) << 4 | hexval(mac_str[2*i+1]);
  #undef hexval

#ifdef __linux__
  if(rtlsim)
  {
    struct sockaddr_un vs_addr, appserver_addr;

    // unlink should fail if nothing is wrong.
    std::string client_path = interface + "client";
    if (unlink(client_path.c_str()) == 0)
      printf("Warning: removing existing unix domain socket %s\n", client_path.c_str());

    // Create the socket.
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (sock < 0)
      throw std::runtime_error("socket() failed: client unix domain socket");

    //setup socket address
    memset(&vs_addr, 0, sizeof(vs_addr));
    memset(&appserver_addr, 0, sizeof(appserver_addr));

    vs_addr.sun_family = AF_UNIX;
    strcpy(vs_addr.sun_path, interface.c_str());

    appserver_addr.sun_family = AF_UNIX;
    strcpy(appserver_addr.sun_path, client_path.c_str());

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
    // setuid root to open a raw socket.
    if (seteuid(0) != 0)
      throw std::runtime_error("couldn't setuid root");

    if ((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
      throw std::runtime_error("socket() failed!");

    if (seteuid(getuid()) != 0)
      throw std::runtime_error("couldn't setuid away from root");

    // get MAC address of local ethernet device
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface.c_str());
    if(ioctl(sock, SIOCGIFHWADDR, (char*)&ifr) == -1)
      throw std::runtime_error("ioctl() failed!");
    memcpy(&src_mac, &ifr.ifr_ifru.ifru_hwaddr.sa_data, sizeof(src_mac));

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sll_ifindex = if_nametoindex(interface.c_str());
    src_addr.sll_family = AF_PACKET;

    if(bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr)) == -1)
      throw std::runtime_error("bind() failed!");

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(struct timeval)) == -1)
      throw std::runtime_error("setsockopt() failed!");
  }
#elif __APPLE__
  assert(!rtlsim);

  char buf[11];
  for (int i = 0; i < 4; i++) {
      sprintf(buf, "/dev/bpf%i", i);
      if ((sock = open(buf, O_RDWR)) != -1)
          break;
  }
  if (sock < 0)
      throw std::runtime_error("open() failed!");

  ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1)
    throw std::runtime_error("getifaddrs() failed");
  for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
  {
    if (interface == ifa->ifa_name && ifa->ifa_addr->sa_family == AF_LINK)
    {
      memcpy(src_mac, LLADDR((sockaddr_dl*)ifa->ifa_addr), sizeof(src_mac));
      break;
    }
  }
  if (ifa == NULL)
    throw std::runtime_error("could not locate interface");
  freeifaddrs(ifaddr);

  struct ifreq bound_if;
  strcpy(bound_if.ifr_name, interface.c_str());
  if (ioctl(sock, BIOCSETIF, &bound_if) == -1)
      throw std::runtime_error("Failed to bind to interface!");

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 50000;
  if (ioctl(sock, BIOCSRTIMEOUT, &tv) == -1)
    throw std::runtime_error("couldn't set socket timeout!");

  int enable = 1;
  if (ioctl(sock, BIOCIMMEDIATE, &enable) == -1)
      throw std::runtime_error("setting immediate mode failed!");
  if (ioctl(sock, BIOCGBLEN, &buffer_len) == -1)
      throw std::runtime_error("cannot get buffer length!");
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
  #ifdef __linux__
    eth_packet_t packet;

    if((bytes = ::read(sock, (char*)&packet, sizeof(packet))) == -1)
    {
      timeouts++;
      debug("read failed (%s)\n",strerror(errno));
      continue;
    }

    if (packet.ethertype != htons(HTIF_ETHERTYPE))
      continue;
    if (memcmp(packet.dst_mac, src_mac, ETHER_ADDR_LEN))
      continue;

    bytes -= 16; //offsetof(eth_header_t, htif_header)
    bytes = std::min(bytes, (ssize_t)(sizeof(packet_header_t) + HTIF_DATA_ALIGN*packet.htif_header.data_size));
    bytes = std::min(bytes, (ssize_t)max_size);
    memcpy(buf, &packet.htif_header, bytes);

    debug("read packet\n");
    for(ssize_t i = 0; i < bytes; i++)
      debug("%02x ",((unsigned char*)buf)[i]);
    debug("\n");

    return bytes;
  #elif __APPLE__
    uint8_t buffer[buffer_len];
    if ((bytes = ::read(sock, buffer, buffer_len)) == -1) {
      timeouts++;
      debug("read failed (%s)\n",strerror(errno));
      continue;
    }
    uint8_t* bufp = buffer;

    while (bufp < buffer + bytes) {
      struct bpf_hdr *hdr = (struct bpf_hdr *) bufp;
      eth_packet_t* packet = (eth_packet_t*)(bufp + hdr->bh_hdrlen);
      bufp += BPF_WORDALIGN(hdr->bh_hdrlen + hdr->bh_caplen);
      assert(hdr->bh_caplen == hdr->bh_datalen);

      if (packet->ethertype != htons(HTIF_ETHERTYPE))
        continue;
      if (memcmp(packet->dst_mac, src_mac, ETHER_ADDR_LEN))
        continue;

      bytes = hdr->bh_caplen;
      bytes -= 16; //offsetof(eth_header_t, htif_header)
      bytes = std::min(bytes, (ssize_t)sizeof(packet_header_t) + HTIF_DATA_ALIGN*packet->htif_header.data_size);
      bytes = std::min(bytes, (ssize_t)max_size);
      memcpy(buf, &packet->htif_header, bytes);

      debug("read packet\n");
      for(ssize_t i = 0; i < bytes; i++)
        debug("%02x ",((unsigned char*)buf)[i]);
      debug("\n");

      return bytes;
    }
  #endif
  }

  return -1;
}

ssize_t htif_eth_t::write(const void* buf, size_t size)
{
  debug("write packet\n");
  for(size_t i = 0; i < size; i++)
    debug("%02x ",((const unsigned char*)buf)[i]);
  debug("\n");

  eth_packet_t eth_packet;
  memcpy(eth_packet.dst_mac, dst_mac, sizeof(dst_mac));
  memcpy(eth_packet.src_mac, src_mac, sizeof(src_mac));
  eth_packet.ethertype = htons(HTIF_ETHERTYPE);
  eth_packet.pad = 0;
  memcpy(&eth_packet.htif_header, buf, size);
  size += 16; //offsetof(eth_packet_t, htif_header)

  #ifdef __linux__
  if(rtlsim)
    return ::write(sock, (char*)&eth_packet, size);
  else
    return ::sendto(sock, (char*)&eth_packet, size, 0,
                    (sockaddr*)&src_addr, sizeof(src_addr));
  #elif __APPLE__
  if (rtlsim)
    return -1;
  else
    return ::write(sock, (char*)&eth_packet, size);
  #endif
}
