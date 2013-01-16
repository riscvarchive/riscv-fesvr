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

  packet_t p(buf, bytes);
  
  if (p.get_header().seqno != expected_seqno)
    throw bad_seqno_error();


  switch (p.get_header().cmd)
  {
    case HTIF_CMD_ACK:
      break;
    case HTIF_CMD_NACK:
      throw packet_error("nack!");
    default:
      throw packet_error("illegal command");
  }

  return p;
}

void htif_t::write_packet(const packet_t& p)
{
  ssize_t bytes = this->write(p.get_packet(), p.get_size());
  if (bytes < (ssize_t)p.get_size())
    throw io_error("write failed");
}

void htif_t::start(int coreid)
{
  if (!started)
  {
    started = true;
    writezeros = true;

    uint32_t first_words[] = {mem_mb(), ncores};
    size_t al = chunk_align();
    uint8_t chunk[(sizeof(first_words)+al-1)/al*al];
    memcpy(chunk, first_words, sizeof(first_words));
    write_chunk(0, sizeof(chunk), chunk);

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

void htif_t::read_chunk(addr_t taddr, size_t len, void* dst)
{
  assert(taddr % chunk_align() == 0);
  assert(len % chunk_align() == 0 && len <= chunk_max_size());

  packet_header_t hdr(HTIF_CMD_READ_MEM, seqno,
                      len/HTIF_DATA_ALIGN, taddr/HTIF_DATA_ALIGN);
  write_packet(hdr);
  packet_t resp = read_packet(seqno);
  seqno++;

  memcpy(dst, resp.get_payload(), len);

}

void htif_t::write_chunk(addr_t taddr, size_t len, const void* src)
{
  assert(taddr % chunk_align() == 0);
  assert(len % chunk_align() == 0 && len <= chunk_max_size());

  bool nonzero = writezeros;
  for (size_t i = 0; i < len && !nonzero; i++)
    nonzero |= ((uint8_t*)src)[i] != 0;

  if (nonzero)
  {
    packet_header_t hdr(HTIF_CMD_WRITE_MEM, seqno,
                        len/HTIF_DATA_ALIGN, taddr/HTIF_DATA_ALIGN);
    write_packet(packet_t(hdr, src, len));
    read_packet(seqno);
    seqno++;
  }
}

reg_t htif_t::read_cr(int coreid, int regnum)
{
  reg_t addr = coreid << 20 | regnum;
  packet_header_t hdr(HTIF_CMD_READ_CONTROL_REG, seqno, 1, addr);
  write_packet(hdr);

  packet_t resp = read_packet(seqno);
  seqno++;

  reg_t val;
  assert(resp.get_payload_size() == sizeof(reg_t));
  memcpy(&val, resp.get_payload(), sizeof(reg_t));
  return val;
}

reg_t htif_t::write_cr(int coreid, int regnum, reg_t val)
{
  reg_t addr = coreid << 20 | regnum;
  packet_header_t hdr(HTIF_CMD_WRITE_CONTROL_REG, seqno, 1, addr);
  write_packet(packet_t(hdr, &val, sizeof(val)));

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
