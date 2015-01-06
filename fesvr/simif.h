#ifndef __SIMIF_H
#define __SIMIF_H

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <queue>
#include "htif_pthread.h"

typedef std::map< uint32_t, uint32_t > map_t;
typedef std::map< uint32_t, std::queue<uint32_t> > qmap_t;
typedef std::map< std::string, std::vector<size_t> > iomap_t;

// Constants
enum DEBUG_CMD {
  STEP_CMD, POKE_CMD, PEEK_CMD, POKEQ_CMD, PEEKQ_CMD, TRACE_CMD, MEM_CMD,
};

enum SNAP_CMD {
  SNAP_FIN, SNAP_STEP, SNAP_POKE, SNAP_EXPECT, SNAP_WRITE, SNAP_READ,
};

enum STEP_RESP {
  RESP_FIN, RESP_TRACE, RESP_PEEKQ,
};

class simif_t
{
  public:
    simif_t(std::vector<std::string> args, uint64_t _max_cycles = -1, bool _trace = false);
    ~simif_t();

    virtual void run();
    void stop() { htif->stop(); }
    bool done() { return is_done; }
    int exit_code() { return htif->exit_code(); }

  private:
    // atomic operations
    virtual void poke(uint32_t value) = 0;
    virtual bool peek_ready() = 0;
    virtual uint32_t peek() = 0;
    // read the target's information
    void read_params(std::string filename);
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
    void read_snap(char* snap);
    void record_snap(char *snap);
    void record_mem();
    virtual void step_htif() { }

    // snapshot information
    std::vector<std::string> signals;
    std::vector<size_t> widths;

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
    // TODO: data structure for snapshots
    FILE *snaps;

    // simulation information
    bool trace; 
    bool pass;
    bool is_done;
    uint64_t t;
    uint64_t fail_t;
    const uint64_t max_cycles;

    // htif information
    htif_pthread_t *htif;
    
  protected:
    std::deque<char> from_htif;
    std::deque<char> to_htif;

    // host's information
    size_t hostlen;
    size_t cmdlen;
    size_t tracelen;
    size_t snaplen;

    // target's information
    size_t htiflen;
    size_t addrlen;
    size_t memlen;
    size_t taglen;

    void open_snap(std::string filename);

    // deqbugging APIs
    void step(size_t n, bool record = true);
    void poke(std::string path, uint64_t value);
    void pokeq(std::string path, uint64_t value);
    uint64_t peek(std::string path);
    uint64_t peekq(std::string path);
    bool peekq_valid(std::string);

    bool expect(std::string path, uint64_t expected);
    bool expect(bool ok, std::string s);

    void load_mem(std::string filename);
    void write_mem(uint64_t addr, uint64_t data);
    uint64_t read_mem(uint64_t addr);

    uint64_t cycles() { return t; }
    uint64_t rand_next(size_t limit); 
};

#endif // __SIMIF_H
