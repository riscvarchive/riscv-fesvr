#include "htif.h"
#include <algorithm>
#include <assert.h>

htif_t::htif_t(int nc)
  : ncores(nc), mem(this), writezeros(true), seqno(1), started(false)
{
}

htif_t::~htif_t()
{
}

packet_t htif_t::read_packet(seqno_t expected_seqno)
{
  size_t max_bytes = sizeof(packet_header_t) + chunk_max_size();
  uint64_t buf[(max_bytes+7)/8];

  ssize_t bytes = this->read(buf, max_bytes);
  if (bytes < (ssize_t)sizeof(packet_header_t))
    throw io_error("read failed");

  packet_header_t hdr = *(packet_header_t*)buf;
  
  if (hdr.seqno != expected_seqno)
    throw bad_seqno_error();

  switch (hdr.cmd)
  {
    case HTIF_CMD_ACK:
      if (hdr.data_size*HTIF_DATA_ALIGN != bytes - sizeof(packet_header_t))
        throw packet_error("bad payload size!");
      break;
    case HTIF_CMD_NACK:
      throw packet_error("nack!");
    default:
      throw packet_error("illegal command");
  }

  packet_t p(hdr);
  p.set_payload((uint8_t*)buf + sizeof(packet_header_t), bytes - sizeof(packet_header_t));
  return p;
}

#include <stdio.h>
void htif_t::write_packet(const packet_t& p)
{
  uint64_t buf[(p.get_size()+7)/8];
  *(packet_header_t*)buf = p.get_header();
  memcpy((uint8_t*)buf + sizeof(packet_header_t), p.get_payload(), p.get_payload_size());

  ssize_t bytes = this->write(buf, p.get_size());
  if (bytes < (ssize_t)p.get_size())
    throw io_error("write failed");
}

void htif_t::start(int coreid)
{
  if (!started)
  {
    started = true;
    writezeros = true;

    uint32_t buf[16] = {mem_mb(), ncores};
    write_chunk(0, sizeof(buf), (uint8_t *)buf);

    for (int i = 0; i < ncores; i++)
    {
      write_cr(i, 29, 1);
      write_cr(i, 10, i);
    }
  }

  write_cr(coreid, 29, 0);
}

void htif_t::stop(int coreid)
{
  write_cr(coreid, 29, 1);
}

void htif_t::read_chunk(addr_t taddr, size_t len, uint8_t* dst)
{
  assert(taddr % chunk_align() == 0);
  assert(len % chunk_align() == 0);

  while (len)
  {
    size_t sz = std::min(len, chunk_max_size());
    packet_t req(packet_header_t(HTIF_CMD_READ_MEM, seqno,
                 sz/HTIF_DATA_ALIGN, taddr/HTIF_DATA_ALIGN));

    write_packet(req);
    packet_t resp = read_packet(seqno);
    seqno++;

    memcpy(dst, resp.get_payload(), sz);

    len -= sz;
    taddr += sz;
    dst += sz;
  }
}

void htif_t::write_chunk(addr_t taddr, size_t len, const uint8_t* src)
{
  assert(taddr % chunk_align() == 0);
  assert(len % chunk_align() == 0);

  uint8_t zeros[chunk_max_size()];
  memset(zeros, 0, chunk_max_size());
  if (src == NULL)
    src = zeros;

  while (len)
  {
    size_t sz = std::min(len, chunk_max_size());

    packet_t req(packet_header_t(HTIF_CMD_WRITE_MEM, seqno,
                 sz/HTIF_DATA_ALIGN, taddr/HTIF_DATA_ALIGN));
    req.set_payload(src, sz);

    if (writezeros || memcmp(zeros, src, sz) != 0)
    {
      write_packet(req);
      read_packet(seqno);
      seqno++;
    }

    len -= sz;
    taddr += sz;
    if (src != zeros)
      src += sz;
  }
}

reg_t htif_t::read_cr(int coreid, int regnum)
{
  packet_t req(packet_header_t(HTIF_CMD_READ_CONTROL_REG, seqno, 1,
                               coreid << 20 | regnum));

  write_packet(req);
  packet_t resp = read_packet(seqno);
  seqno++;

  reg_t val;
  assert(resp.get_payload_size() == sizeof(reg_t));
  memcpy(&val, resp.get_payload(), sizeof(reg_t));
  return val;
}

reg_t htif_t::write_cr(int coreid, int regnum, reg_t val)
{
  packet_t req(packet_header_t(HTIF_CMD_WRITE_CONTROL_REG, seqno, 1,
                               coreid << 20 | regnum));
  req.set_payload(&val, sizeof(reg_t));

  write_packet(req);
  packet_t resp = read_packet(seqno);
  seqno++;

  assert(resp.get_payload_size() == sizeof(reg_t));
  memcpy(&val, resp.get_payload(), sizeof(reg_t));
  return val;
}

void htif_t::assume0init(bool val)
{
  writezeros = !val;
}
