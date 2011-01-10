#include <algorithm>
#include <sys/types.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include "common.h"
#include "interface.h"
#include "htif_rs232.h"
#include "memif.h"

#define HTIF_MAX_DATA_SIZE RS232_MAX_DATA_SIZE
#include "htif-packet.h"

#define debug(...)
//#define debug(...) fprintf(stderr,__VA_ARGS__)

htif_rs232_t::htif_rs232_t(const char* tty)
: seqno(1)
{
  debug("opening %s\n",tty);
  fd = open(tty,O_RDWR|O_NOCTTY);
  assert(fd != -1);

  struct termios tio;
  memset(&tio,0,sizeof(tio));

  tio.c_cflag = B9600;
  tio.c_iflag = IGNPAR;
  tio.c_oflag = 0;
  tio.c_lflag = 0;
  tio.c_cc[VTIME] = 0;
  tio.c_cc[VMIN] = 1; // read in 8 byte chunks

  tcflush(fd, TCIOFLUSH);
  tcsetattr(fd, TCSANOW, &tio);
}

htif_rs232_t::~htif_rs232_t()
{
}

void htif_rs232_t::read_packet(packet_t* p, int expected_seqno)
{
  debug("read packet\n");
  int bytes = 16;
  for(int i = 0; i < bytes; i++)
  {
    assert(read(fd, ((char*)p)+i, 1) == 1);
    if(i == 15)
      bytes += p->data_size;
    debug("%x ",((unsigned char*)p)[i]);
    fflush(stdout);
  }
  debug("\n");

  if (bytes == -1 || bytes < (int)offsetof(packet_t, data))
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

void htif_rs232_t::write_packet(const packet_t* p)
{
  int size = offsetof(packet_t, data);
  if(p->cmd == HTIF_CMD_WRITE_MEM || p->cmd == HTIF_CMD_WRITE_CONTROL_REG)
    size += p->data_size;
    
  debug("write packet\n");
  for(int i = 0; i < size; i++)
    debug("%x ",((const unsigned char*)p)[i]);
  debug("\n");

  int bytes = write(fd, p, size);
  if (bytes < size)
    throw io_error("write failed");
}

void htif_rs232_t::start()
{
  packet_t p = {HTIF_CMD_START, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_rs232_t::stop()
{
  packet_t p = {HTIF_CMD_STOP, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_rs232_t::read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd)
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
    size_t sz = std::min(len, RS232_MAX_DATA_SIZE);

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

void htif_rs232_t::write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd)
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
    size_t sz = std::min(len, RS232_MAX_DATA_SIZE);

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

reg_t htif_rs232_t::read_cr(int coreid, int regnum)
{
  reg_t val;
  do
  {
    read_chunk((addr_t)coreid<<32|regnum, sizeof(reg_t), (uint8_t*)&val, IF_CREG);
  } while (val == 0);
  return val;
}

void htif_rs232_t::write_cr(int coreid, int regnum, reg_t val)
{
  write_chunk((addr_t)coreid<<32|regnum, sizeof(reg_t), (uint8_t*)&val, IF_CREG);
}

size_t htif_rs232_t::chunk_align()
{
  return RS232_DATA_ALIGN;
}
