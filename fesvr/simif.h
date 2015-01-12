#ifndef __SIMIF_H
#define __SIMIF_H

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <queue>
#include "htif_pthread.h"
#include "sample.h"

typedef std::map< uint32_t, uint32_t > map_t;
typedef std::map< uint32_t, std::queue<uint32_t> > qmap_t;
typedef std::map< std::string, std::vector<size_t> > iomap_t;

// Constants
enum DEBUG_CMD {
  STEP_CMD, POKE_CMD, PEEK_CMD, POKEQ_CMD, PEEKQ_CMD, TRACE_CMD, MEM_CMD,
};

enum STEP_RESP {
  RESP_FIN, RESP_TRACE, RESP_PEEKQ,
};

class simif_t
{
  public:
    simif_t(std::vector<std::string> args, bool _log = false);
    ~simif_t();

    virtual int run();
    void stop() { htif->stop(); }
    bool done() const { return is_done; }
    int exit_code() const { return exitcode; }

  private:
    // atomic operations
    virtual void poke(uint32_t value) = 0;
    virtual bool peek_ready() = 0;
    virtual uint32_t peek() = 0;

    // read the target's information
    void read_io_map(std::string filename);
    void read_chain_map(std::string filename);

    // simif operation
    void poke_steps(size_t n, bool record = true);
    void poke_all();
    void peek_all();
    void pokeq_all();
    void peekq_all();
    void peek_trace();
    void trace_qout();
    void trace_mem();
    void record_io();
    void read_snap(uint64_t sample_idx);
    void record_mem();
    virtual void serve_htif(const size_t size) { }

    // io information
    iomap_t qin_map;
    iomap_t qout_map;
    iomap_t win_map;
    iomap_t wout_map;
    size_t qin_num;
    size_t qout_num;
    size_t win_num;
    size_t wout_num;

    // poke & peek mappings
    map_t poke_map;
    map_t peek_map;
    qmap_t pokeq_map;
    qmap_t peekq_map;

    // memory trace
    map_t mem_writes;
    map_t mem_reads;

    // snapshot contends
    sample_t** samples;

    // simulation information
    const bool log; 
    bool pass;
    bool is_done;
    int exitcode;   
    uint64_t t;
    uint64_t fail_t;
    uint64_t snap_len;
    uint64_t max_cycles;
    uint64_t step_size;
    uint64_t sample_num;

    // constants
    virtual size_t htiflen() const { return 16; }
    virtual size_t hostlen() const { return 32; }
    virtual size_t addrlen() const { return 26; } 
    virtual size_t memlen() const { return 128; } 
    virtual size_t taglen() const { return 5; }
    virtual size_t cmdlen() const { return 4; }
    virtual size_t tracelen() const { return 16; }

    htif_pthread_t *htif;
    
    std::vector<std::string> hargs;
    std::vector<std::string> targs;
    std::string loadmem;
    std::string prefix;

  protected:
    std::deque<char> from_htif;
    std::deque<char> to_htif;

    // Simulation APIs
    void step(size_t n);
    void poke(std::string path, uint64_t value);
    void pokeq(std::string path, uint64_t value);
    uint64_t peek(std::string path);
    uint64_t peekq(std::string path);
    bool peekq_valid(std::string);

    bool expect(std::string path, uint64_t expected);
    bool expect(bool ok, std::string s);

    void load_mem();
    void write_mem(uint64_t addr, uint64_t data);
    uint64_t read_mem(uint64_t addr);

    uint64_t cycles() { return t; }
    uint64_t rand_next(size_t limit); 
};

#endif // __SIMIF_H
