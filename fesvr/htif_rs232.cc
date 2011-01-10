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

htif_rs232_t::htif_rs232_t(const char* tty)
: seqno(1)
{
  fd = open(tty,O_RDWR|O_NOCTTY);
  assert(fd != -1);

  struct termios tio;
  memset(&tio,0,sizeof(tio));

  tio.c_cflag = B38400;
  tio.c_iflag = IGNPAR;
  tio.c_oflag = 0;
  tio.c_lflag = 0;
  tio.c_cc[VTIME] = 0;
  tio.c_cc[VMIN] = 1; // read in 8 byte chunks

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &tio);
}

htif_rs232_t::~htif_rs232_t()
{
}

void htif_rs232_t::read_packet(rs232_packet_t* p, int expected_seqno)
{
  printf("read packet\n");
  int bytes = 16;
  for(int i = 0; i < bytes; i++)
  {
    assert(read(fd, ((char*)p)+i, 1) == 1);
    if(i == 15)
      bytes += p->data_size;
  }

  printf("%d\n",bytes);
  for(int i = 0; i < bytes; i++)
    printf("%x ",((unsigned char*)p)[i]);
  printf("\n");

  if (bytes == -1 || bytes < (int)offsetof(rs232_packet_t, data))
    throw rs232_io_error("read failed");
  if (p->seqno != expected_seqno)
    throw rs232_bad_seqno_error(); 
  switch (p->cmd)
  {
    case RS232_CMD_ACK:
      if (p->data_size != bytes - offsetof(rs232_packet_t, data))
        throw rs232_packet_error("bad payload size!");
      break;
    case RS232_CMD_NACK:
      throw rs232_packet_error("nack!");
    default:
      throw rs232_packet_error("illegal command");
  }
}

void htif_rs232_t::write_packet(const rs232_packet_t* p)
{
  int size = offsetof(rs232_packet_t, data);
  if(p->cmd == RS232_CMD_WRITE_MEM || p->cmd == RS232_CMD_WRITE_CONTROL_REG)
    size += p->data_size;
    
  printf("write packet\n");
  int bytes = write(fd, p, size);
  if (bytes < size)
    throw rs232_io_error("write failed");
}

void htif_rs232_t::start()
{
  int start = 0x40000000;
  int len = 1024;
  uint8_t buf[len];
  read_chunk(start, len, buf, IF_MEM);

  rs232_packet_t p = {RS232_CMD_START, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_rs232_t::stop()
{
  rs232_packet_t p = {RS232_CMD_STOP, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_rs232_t::read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd)
{
  demand(cmd == IF_CREG || taddr % chunk_align() == 0, "taddr=%016lx read_chunk misaligned", taddr);
  demand(len % chunk_align() == 0, "len=%ld read_chunk misaligned", len);

  rs232_packet_t req;
  rs232_packet_t resp;

  if (cmd == IF_MEM)
    req.cmd = RS232_CMD_READ_MEM;
  else if (cmd == IF_CREG)
    req.cmd = RS232_CMD_READ_CONTROL_REG;
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

  rs232_packet_t req;
  rs232_packet_t resp;

  if (cmd == IF_MEM)
    req.cmd = RS232_CMD_WRITE_MEM;
  else if (cmd == IF_CREG)
    req.cmd = RS232_CMD_WRITE_CONTROL_REG;
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
