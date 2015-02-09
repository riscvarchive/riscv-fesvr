#ifndef __SIMIF_PTHREAD_H
#define __SIMIF_PTHREAD_H

#include <queue>
#include "simif.h"
#include "context.h"

class simif_pthread_t : public simif_t
{
  public:
    simif_pthread_t(
      std::vector<std::string> args, 
      std::string prefix = "Top", 
      bool log = false,
      bool check_sample = false,
      bool has_htif = true);
    ~simif_pthread_t() { }

    // target interface
    void send(uint32_t);
    uint32_t recv();
    bool recv_nonblocking(uint32_t& value);

    // htif interface
    void send_to_htif(const void *buf, size_t size);
    void recv_from_htif(void *buf, size_t size);
    bool recv_from_htif_nonblocking(void *buf, size_t size);

  private:
    virtual void poke_host(uint32_t value);
    virtual bool peek_host_ready();
    virtual uint32_t peek_host();

    virtual void serve_htif(size_t size);

    // threads for simulation
    context_t host;
    context_t* target;
    std::queue<uint32_t> th_data;
    std::queue<uint32_t> ht_data;

    static void thread_main(void* simif);
};

#endif // __SIMIF_PTHREAD_H
