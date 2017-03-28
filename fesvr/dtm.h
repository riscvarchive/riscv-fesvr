#ifndef _ROCKET_DTM_H
#define _ROCKET_DTM_H

#include "htif.h"
#include "context.h"
#include <stdint.h>
#include <queue>
#include <semaphore.h>
#include <vector>
#include <string>
#include <stdlib.h>

// abstract debug transport module
class dtm_t : public htif_t
{
 public:
  dtm_t(const std::vector<std::string>& args);
  ~dtm_t();

  struct req {
    uint32_t addr;
    uint32_t op;
    // MW: This is only 32 bits now vs 34, but probably fine to keep it as 64
    uint64_t data;
  };

  struct resp {
    uint32_t resp;
    // MW: Again only 32 bits now
    uint64_t data;
  };
  
  void tick(
    bool  req_ready,
    bool  resp_valid,
    resp  resp_bits
  );
  // Akin to tick, but the target thread returns a response on every invocation
  void return_resp(
    resp  resp_bits
  );

  bool req_valid() { return req_wait; }
  req req_bits() { return req_buf; }
  bool resp_ready() { return true; }

  uint64_t read(uint32_t addr);
  uint64_t write(uint32_t addr, uint64_t data);
  void nop();

  uint64_t read_csr(unsigned which);
  uint64_t write_csr(unsigned which, uint64_t data);
  uint64_t clear_csr(unsigned which, uint64_t data);
  uint64_t set_csr(unsigned which, uint64_t data);
  void fence_i();

  void producer_thread();

 private:
  context_t host;
  context_t* target;
  pthread_t producer;
  sem_t req_produce;
  sem_t req_consume;
  sem_t resp_produce;
  sem_t resp_consume;
  req req_buf;
  resp resp_buf;

  uint32_t run_program(const uint32_t program[], size_t n, uint8_t result);
  
  uint64_t modify_csr(unsigned which, uint64_t data, uint32_t type);

  void read_chunk(addr_t taddr, size_t len, void* dst) override;
  void write_chunk(addr_t taddr, size_t len, const void* src) override;
  void clear_chunk(addr_t taddr, size_t len) override;
  size_t chunk_align() override;
  size_t chunk_max_size() override;
  void reset() override;
  void idle() override;

  bool req_wait;
  bool resp_wait;
  uint32_t dminfo;
  // [MW] some information you may need is in abstractcs register
  uint32_t abstractcs;
  
  uint32_t xlen;

  static const int max_idle_cycles = 10000;

  // [MW] These addresses aren't defined in the spec
  // anymore. There are ways to figure them out e.g. at
  // start up by reading some registers or running programs.
  // Also, you might want to distinguish between ram_base,
  // which is basically the old ram_base, and "data_base", which
  // is for data transfer only. You could just ignore data_base and
  // do everything through ram_base as this code is currently doing,
  // or you can use the "abstract commands" to do the data transfer to GPRs
  // which basically means you don't need to figure out any
  // of these addresses.
  
  // Depending how generic you want this to be, 
  // you might want to determine these by querying the implementation
  // vs hard-coding them here, or by using abstract commands you don't even
  // need these.
  // You can determine this by running doing aiupc to s0, then using the abstract
  // command to transfer it out. But, again you probably don't even need to
  // know this if you do use abstract command.
  size_t ram_base()  { return 0x400; }

  // MW you can read this in hartinfo.
  size_t data_base() { return 0x400; }
  
  // MW you don't need this anymore. Use ebreak to indicate end of programs.
  // Debugger has to explicitly tell the hart to resume, it can't resume itself
  // by executing code like this anymore.
  //size_t rom_ret()  { return 0x804; }

  // [MW] Updated this to match current spec
  //size_t ram_words() { return ((dminfo >> 10) & 63) + 1; }
  size_t ram_words() { return ((abstractcs >> 24) & 0x1F) }
  size_t data_words()    { return ((abstractcs >> 0)  & 0xF) }
                                  
  uint32_t get_xlen();
  uint64_t do_command(dtm_t::req r);

  void parse_args(const std::vector<std::string>& args);
  void register_devices();
  void start_host_thread();

  friend class memif_t;
};

#endif
