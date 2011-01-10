#include <algorithm>
#include "common.h"
#include "interface.h"
#include "htif_rtlsim.h"

htif_rtlsim_t::htif_rtlsim_t(int _fdin, int _fdout)
: fdin(_fdin), fdout(_fdout), seqno(1)
{
}

htif_rtlsim_t::~htif_rtlsim_t()
{
}

void htif_rtlsim_t::read_packet(rtl_packet_t* p, int expected_seqno)
{
  int bytes = read(fdin, p, sizeof(rtl_packet_t));
  if (bytes == -1 || bytes < (int)offsetof(rtl_packet_t, data))
    throw rtl_io_error("read failed");
  if (p->seqno != expected_seqno)
    throw rtl_bad_seqno_error(); 
  switch (p->cmd)
  {
    case RTL_CMD_ACK:
      if (p->data_size != bytes - offsetof(rtl_packet_t, data))
        throw rtl_packet_error("bad payload size!");
      break;
    case RTL_CMD_NACK:
      throw rtl_packet_error("nack!");
    default:
      throw rtl_packet_error("illegal command");
  }
}

void htif_rtlsim_t::write_packet(const rtl_packet_t* p)
{
  int size = offsetof(rtl_packet_t, data);
  if(p->cmd == RTL_CMD_WRITE_MEM || p->cmd == RTL_CMD_WRITE_CONTROL_REG)
    size += p->data_size;
    
  int bytes = write(fdout, p, size);
  if (bytes < size)
    throw rtl_io_error("write failed");
}

void htif_rtlsim_t::start()
{
  rtl_packet_t p = {RTL_CMD_START, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_rtlsim_t::stop()
{
  rtl_packet_t p = {RTL_CMD_STOP, seqno, 0, 0};
  write_packet(&p);
  read_packet(&p, seqno);
  seqno++;
}

void htif_rtlsim_t::read_chunk(addr_t taddr, size_t len, uint8_t* dst, int cmd)
{
  demand(cmd == IF_CREG || taddr % chunk_align() == 0, "taddr=%016lx read_chunk misaligned", taddr);
  demand(len % chunk_align() == 0, "len=%ld read_chunk misaligned", len);

  rtl_packet_t req;
  rtl_packet_t resp;

  if (cmd == IF_MEM)
    req.cmd = RTL_CMD_READ_MEM;
  else if (cmd == IF_CREG)
    req.cmd = RTL_CMD_READ_CONTROL_REG;
  else
    demand(0, "unreachable");

  while (len)
  {
    size_t sz = std::min(len, RTL_MAX_DATA_SIZE);

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

void htif_rtlsim_t::write_chunk(addr_t taddr, size_t len, const uint8_t* src, int cmd)
{
  demand(cmd == IF_CREG || taddr % chunk_align() == 0, "taddr=%016lx write_chunk misaligned", taddr);
  demand(len % chunk_align() == 0, "len=%ld write_chunk misaligned", len);

  rtl_packet_t req;
  rtl_packet_t resp;

  if (cmd == IF_MEM)
    req.cmd = RTL_CMD_WRITE_MEM;
  else if (cmd == IF_CREG)
    req.cmd = RTL_CMD_WRITE_CONTROL_REG;
  else
    demand(0, "unreachable");

  while (len)
  {
    size_t sz = std::min(len, RTL_MAX_DATA_SIZE);

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

reg_t htif_rtlsim_t::read_cr(int coreid, int regnum)
{
  reg_t val;
  do
  {
    read_chunk((addr_t)coreid<<32|regnum, sizeof(reg_t), (uint8_t*)&val, IF_CREG);
  } while (val == 0);
  return val;
}

void htif_rtlsim_t::write_cr(int coreid, int regnum, reg_t val)
{
  write_chunk((addr_t)coreid<<32|regnum, sizeof(reg_t), (uint8_t*)&val, IF_CREG);
}

size_t htif_rtlsim_t::chunk_align()
{
  return RTL_DATA_ALIGN;
}
