#include "htif.h"
#include "elfloader.h"
#include <algorithm>
#include <assert.h>
#include <termios.h>
#include <vector>
#include <queue>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

htif_t::htif_t(const std::vector<std::string>& args)
  : exitcode(0), mem(this), syscall(this), seqno(1), started(false),
    _mem_mb(0), _num_cores(0), old_tios(0), sig_addr(0), sig_len(0)
{
  size_t i;
  for (i = 0; i < args.size(); i++)
    if (args[i].length() && args[i][0] != '-' && args[i][0] != '+')
      break;

  hargs.insert(hargs.begin(), args.begin(), args.begin() + i);
  targs.insert(targs.begin(), args.begin() + i, args.end());
}

htif_t::~htif_t()
{
  termios_destroy();
}

packet_t htif_t::read_packet(seqno_t expected_seqno)
{
  while (1)
  {
    if (read_buf.size() >= sizeof(packet_header_t))
    {
      packet_header_t hdr(&read_buf[0]);
      if (read_buf.size() >= hdr.get_packet_size())
      {
        packet_t p(&read_buf[0]);
        switch (p.get_header().cmd)
        {
          case HTIF_CMD_ACK:
            break;
          case HTIF_CMD_NACK:
            throw packet_error("nack!");
          default:
            throw packet_error("illegal command " + itoa(p.get_header().cmd));
        }
        read_buf.erase(read_buf.begin(), read_buf.begin() + hdr.get_packet_size());
        return p;
      }
    }
    size_t old_size = read_buf.size();
    size_t max_size = sizeof(packet_header_t) + chunk_max_size();
    read_buf.resize(old_size + max_size);
    ssize_t this_size = this->read(&read_buf[old_size], max_size);
    if (this_size < 0)
      throw io_error("read failed");
    read_buf.resize(old_size + this_size);
  }
}

void htif_t::write_packet(const packet_t& p)
{
  for (size_t pos = 0; pos < p.get_size(); )
  {
    ssize_t bytes = this->write(p.get_packet() + pos, p.get_size() - pos);
    if (bytes < 0)
      throw io_error("write failed");
    pos += bytes;
  }
}

void htif_t::start()
{
  assert(!started);
  started = true;

  load_program();
  reset();
}

void htif_t::load_program()
{
  if (targs.size() == 0 || targs[0] == "none")
    return;

  const char* rvpath = getenv("RISCV");
  std::string p1 = rvpath ? rvpath + ("/target/bin/" + targs[0]) : "";
  std::string p2 = targs[0];
  if (access(p1.c_str(), F_OK))
  {
    p1 = p2;
    if (access(p1.c_str(), F_OK) != 0)
      throw std::runtime_error("could not open " + targs[0]);
  }

  std::map<std::string, uint64_t> symbols = load_elf(p1.c_str(), &mem);

  // detect torture tests so we can print the memory signature at the end
  if (symbols.count("begin_signature") && symbols.count("end_signature"))
  {
    sig_addr = symbols["begin_signature"];
    sig_len = symbols["end_signature"] - sig_addr;
  }
}

void htif_t::reset()
{
  uint32_t first_words[] = {mem_mb(), num_cores()};
  size_t al = chunk_align();
  uint8_t chunk[(sizeof(first_words)+al-1)/al*al];
  memcpy(chunk, first_words, sizeof(first_words));
  write_chunk(0, sizeof(chunk), chunk);

  for (uint32_t i = 0; i < num_cores(); i++)
  {
    write_cr(i, 29, 1);
    write_cr(i, 10, coremap(i));
    write_cr(i, 29, 0);
  }
}

uint32_t htif_t::coremap(uint32_t x)
{
  if (coremap_pool.size() == 0)
  {
    coremap_pool.resize(num_cores());
    for (uint32_t i = 0; i < num_cores(); i++)
      coremap_pool[i] = i;

    if (std::find(hargs.begin(), hargs.end(), "+coremap-reverse") != hargs.end())
      std::reverse(coremap_pool.begin(), coremap_pool.end());
    else if (std::find(hargs.begin(), hargs.end(), "+coremap-random") != hargs.end())
    {
      std::srand(time(NULL) ^ getpid());
      std::random_shuffle(coremap_pool.begin(), coremap_pool.end());
    }

    if (std::find(hargs.begin(), hargs.end(), "+print-coremap") != hargs.end())
    {
      std::cerr << "core map:" << std::endl;
      for (uint32_t i = 0; i < num_cores(); i++)
        std::cerr << i << ", " << coremap_pool[i] << std::endl;
    }
  }

  return coremap_pool[x];
}

void htif_t::stop()
{
  if (sig_len) // print final torture test signature
  {
    std::vector<uint8_t> buf(sig_len);
    mem.read(sig_addr, sig_len, &buf[0]);

    const addr_t incr = 16;
    assert(sig_len % incr == 0);
    for (addr_t i = 0; i < sig_len; i += incr)
    {
      for (addr_t j = incr; j > 0; j--)
        printf("%02x", buf[i+j-1]);
      printf("\n");
    }
  }

  for (uint32_t i = 0, nc = num_cores(); i < nc; i++)
    write_cr(i, 29, 1);
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

  bool nonzero = started || !assume0init();
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

reg_t htif_t::read_cr(uint32_t coreid, uint16_t regnum)
{
  reg_t addr = (reg_t)coreid << 20 | regnum;
  packet_header_t hdr(HTIF_CMD_READ_CONTROL_REG, seqno, 1, addr);
  write_packet(hdr);

  packet_t resp = read_packet(seqno);
  seqno++;

  reg_t val;
  assert(resp.get_payload_size() == sizeof(reg_t));
  memcpy(&val, resp.get_payload(), sizeof(reg_t));
  return val;
}

reg_t htif_t::write_cr(uint32_t coreid, uint16_t regnum, reg_t val)
{
  reg_t addr = (reg_t)coreid << 20 | regnum;
  packet_header_t hdr(HTIF_CMD_WRITE_CONTROL_REG, seqno, 1, addr);
  write_packet(packet_t(hdr, &val, sizeof(val)));

  packet_t resp = read_packet(seqno);
  seqno++;

  assert(resp.get_payload_size() == sizeof(reg_t));
  memcpy(&val, resp.get_payload(), sizeof(reg_t));
  return val;
}

struct htif_t::core_status
{
  core_status() : poll_keyboard(0) {}
  std::queue<reg_t> fromhost;
  reg_t poll_keyboard;
};

int htif_t::run()
{
  start();
  std::vector<core_status> status(num_cores());

  while (!done())
  {
    for (uint32_t coreid = 0; coreid < num_cores(); coreid++)
    {
      poll_tohost(coreid, &status[coreid]);
      poll_keyboard(coreid, &status[coreid]);
      drain_fromhost_writes(coreid, &status[coreid], false);
    }
  }

  for (uint32_t coreid = 0; coreid < num_cores(); coreid++)
    drain_fromhost_writes(coreid, &status[coreid], true);

  stop();

  return exit_code();
}

void htif_t::poll_tohost(int coreid, core_status* s)
{
  reg_t tohost = read_cr(coreid, 30);
  if (tohost == 0)
    return;

  #define DEVICE(reg) (((reg) >> 56) & 0xFF)
  #define INTERRUPT(reg) (((reg) >> 48) & 0x80)
  #define COMMAND(reg) (((reg) >> 48) & 0x7F)
  #define PAYLOAD(reg) ((reg) & 0xFFFFFFFFFFFF)

  switch (DEVICE(tohost))
  {
    case 0: // proxy syscall or test pass/fail
    {
      if (PAYLOAD(tohost) & 1) // test pass/fail
      {
        exitcode = PAYLOAD(tohost);
        if (exit_code())
          std::cerr << "*** FAILED *** (tohost = " << exit_code() << ")" << std::endl;
      }
      else
        syscall.dispatch(PAYLOAD(tohost));
      s->fromhost.push(1);
      break;
    }
    case 1: // console
    {
      switch (COMMAND(tohost))
      {
        case 0: // read
          if (s->poll_keyboard)
            return;
          s->poll_keyboard = tohost;
          break;
        case 1: // write
        {
          unsigned char ch = PAYLOAD(tohost);
          if (ch == 0)
            exitcode = 1;
          assert(::write(1, &ch, 1) == 1);
          break;
        }
      };
      break;
    }
  };
}

void htif_t::poll_keyboard(int coreid, core_status* s)
{
  if (s->poll_keyboard)
  {
    termios_init();
    unsigned char ch;
    if (::read(0, &ch, 1) == 1)
    {
      s->fromhost.push(s->poll_keyboard | ch);
      s->poll_keyboard = 0;
    }
    else if (!INTERRUPT(s->poll_keyboard))
    {
      s->fromhost.push(s->poll_keyboard | 0x100);
      s->poll_keyboard = 0;
    }
  }
}

void htif_t::drain_fromhost_writes(int coreid, core_status* s, bool sync)
{
  while (!s->fromhost.empty())
  {
    reg_t value = s->fromhost.front();
    if (write_cr(coreid, 31, value) == 0)
    {
      if (INTERRUPT(value))
        write_cr(coreid, 9, 1);
      s->fromhost.pop();
    }
    if (!sync) break;
  }
}

void htif_t::termios_init()
{
  if (!old_tios)
  {
    old_tios = new struct termios;
    tcgetattr(0, old_tios);

    struct termios new_tios = *old_tios;
    new_tios.c_lflag &= ~(ICANON | ECHO);
    new_tios.c_cc[VMIN] = 0;
    tcsetattr(0, TCSANOW, &new_tios);
  }
}

void htif_t::termios_destroy()
{
  if (old_tios)
  {
    tcsetattr(0, TCSANOW, old_tios);
    delete old_tios;
  }
}

uint32_t htif_t::num_cores()
{
  if (_num_cores == 0)
    _num_cores = read_cr(-1, 0);
  return _num_cores;
}

uint32_t htif_t::mem_mb()
{
  if (_mem_mb == 0)
    _mem_mb = read_cr(-1, 1);
  return _mem_mb;
}
