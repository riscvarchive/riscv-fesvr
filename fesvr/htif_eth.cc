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

#ifdef __linux__
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
#elif __APPLE__
  if (rtlsim) {
    assert(0); // Unimplemented
  } else {
    char buf[11];
    for (int i = 0; i < 4; i++) {
        sprintf(buf, "/dev/bpf%i", i);
        if ((sock = open(buf, O_RDWR)) != -1)
            break;
    }
    if (sock < 0)
        throw std::runtime_error("open() failed!");

    struct ifreq bound_if;
    strcpy(bound_if.ifr_name, interface);
    if (ioctl(sock, BIOCSETIF, &bound_if) == -1)
        throw std::runtime_error("Failed to bind to interface!");

    int enable = 1;
    if (ioctl(sock, BIOCIMMEDIATE, &enable) == -1)
        throw std::runtime_error("setting immediate mode failed!");
    if (ioctl(sock, BIOCGBLEN, &buffer_len) == -1)
        throw std::runtime_error("cannot get buffer length!");
  }
#else
  assert(0);
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

    if(packet.ethertype != htons(HTIF_ETHERTYPE))
    {
      debug("wrong ethertype\n");
      continue;
    }

    bytes -= 16; //offsetof(eth_header_t, htif_header)
    bytes = std::min(bytes, (ssize_t)sizeof(packet_header_t) + HTIF_DATA_ALIGN*packet.htif_header.data_size);
    bytes = std::min(bytes, (ssize_t)max_size);
    memcpy(buf, &packet.htif_header, bytes);

    debug("read packet\n");
    for(ssize_t i = 0; i < bytes; i++)
      debug("%02x ",((unsigned char*)buf)[i]);
    debug("\n");

    return bytes;
  #elif __APPLE__
    static unsigned char *buffer = (unsigned char *) malloc(buffer_len);
    if ((bytes = ::read(sock, buffer, buffer_len)) == -1) {
      timeouts++;
      debug("read failed (%s)\n",strerror(errno));
      continue;
    }
    ssize_t checking_bytes = bytes;

    while (checking_bytes > 0) {
      bool filter = false;
      struct bpf_hdr *hdr = (struct bpf_hdr *) buffer;
      int packet_bytes = BPF_WORDALIGN(hdr->bh_hdrlen + hdr->bh_caplen);
      int index = hdr->bh_hdrlen + ETHER_ADDR_LEN; // Set to src first byte

      assert(hdr->bh_caplen == hdr->bh_datalen); // Unimplemented
      
      int src_mac = -1;
      if (memcmp(buffer + index, &src_mac, ETHER_ADDR_LEN))
          filter = true;
      index += ETHER_ADDR_LEN;
      short int type = htons(HTIF_ETHERTYPE);
      if (memcmp(buffer + index, &type, ETHER_TYPE_LEN))
          filter = true;
      index += ETHER_TYPE_LEN;

      if (!filter) {
        bytes = std::min((size_t)hdr->bh_caplen, max_size);
        memcpy(buf, buffer + index, bytes);

        debug("read packet\n");
        for(ssize_t i = 0; i < bytes; i++)
          debug("%02x ",((unsigned char*)buf)[i]);
        debug("\n");

        return bytes;
      }

      buffer += packet_bytes;
      checking_bytes -= packet_bytes;
    }
  #endif
  }

  return -1;
}

ssize_t htif_eth_t::write(const void* buf, size_t size, bool hack_reset)
{
  debug("write packet\n");
  for(size_t i = 0; i < size; i++)
    debug("%02x ",((const unsigned char*)buf)[i]);
  debug("\n");

  int loop = 1;
  if (hack_reset) loop = 2;

  ssize_t len;
  for (int i=0; i<loop; i++)
  {
    eth_packet_t eth_packet;
    memset(eth_packet.dst_mac, -1, sizeof(src_mac));
    memcpy(eth_packet.src_mac, src_mac, sizeof(src_mac));
    // 0x8889 is chip_reset = 1'b1
    // 0x888A is chip_reset = 1'b0
    // which in turn resets the core's reset
    // this is a workaround for the reset problem
    eth_packet.ethertype = htons(HTIF_ETHERTYPE+i+1);
    eth_packet.pad = 0;
    memcpy(&eth_packet.htif_header, buf, size);
    size += 16; //offsetof(eth_packet_t, htif_header)

    #ifdef __linux__
    if(rtlsim)
      len = ::write(sock, (char*)&eth_packet, size);
    else
      len = ::sendto(sock, (char*)&eth_packet, size, 0,
                      (sockaddr*)&src_addr, sizeof(src_addr));
    #elif __APPLE__
    if (rtlsim)
      len = -1;
    else
      len = ::write(sock, (char*)&eth_packet, size);
    #endif
  }
  return len;
}
