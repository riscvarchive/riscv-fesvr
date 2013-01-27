#ifndef __HTIF_H
#define __HTIF_H

#include "memif.h"
#include "syscall.h"
#include <string.h>
#include <vector>

class htif_t
{
 public:
  htif_t(const std::vector<std::string>& target_args);
  virtual ~htif_t();

  virtual void start();
  virtual void stop();

  int run();
  bool done() { return exitcode & 1; }
  int exit_code() { return exitcode >> 1; }

  virtual reg_t read_cr(uint32_t coreid, uint16_t regnum);
  virtual reg_t write_cr(uint32_t coreid, uint16_t regnum, reg_t val);

  virtual memif_t& memif() { return mem; }
  virtual uint32_t num_cores();
  virtual uint32_t mem_mb();

 protected:
  virtual void read_chunk(addr_t taddr, size_t len, void* dst);
  virtual void write_chunk(addr_t taddr, size_t len, const void* src);

  virtual size_t chunk_align() = 0;
  virtual size_t chunk_max_size() = 0;
  virtual bool assume0init() { return false; }

  virtual ssize_t read(void* buf, size_t max_size) = 0;
  virtual ssize_t write(const void* buf, size_t size) = 0;

  const std::vector<std::string>& host_args() { return hargs; }
  int exitcode;

  virtual void load_program();
  virtual void reset();
  virtual uint32_t coremap(uint32_t);

 private:
  memif_t mem;
  syscall_t syscall;
  bool writezeros;
  seqno_t seqno;
  bool started;
  uint32_t _mem_mb;
  uint32_t _num_cores;
  struct termios* old_tios;
  std::vector<std::string> hargs;
  std::vector<std::string> targs;
  std::vector<uint32_t> coremap_pool;
  addr_t sig_addr; // torture
  addr_t sig_len; // torture

  std::vector<char> read_buf;
  virtual packet_t read_packet(seqno_t expected_seqno);
  virtual void write_packet(const packet_t& packet);

  void termios_init();
  void termios_destroy();

  friend class memif_t;
  friend class syscall_t;

  struct core_status;
  const std::vector<std::string>& target_args() { return targs; }
  void poll_tohost(int coreid, core_status* s);
  void poll_keyboard(int coreid, core_status* s);
  void drain_fromhost_writes(int coreid, core_status* s, bool sync);
};

#endif // __HTIF_H
