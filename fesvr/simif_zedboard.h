#ifndef __SIMIF_PTHREAD_H
#define __SIMIF_PTHREAD_H

#include "simif.h"

class simif_zedboard_t : public simif_t
{
  public:
    simif_zedboard_t(std::vector<std::string> args, uint64_t _max_cycles = -1, bool _trace = false);
    ~simif_zedboard_t() { }

  private:
    virtual void poke(uint32_t value);
    virtual bool peek_ready();
    virtual uint32_t peek();

    void poke_htif(uint32_t value);
    bool peek_htif_ready();
    uint32_t peek_htif();

    virtual void step_htif();

    volatile uintptr_t* dev_vaddr;
    const static uintptr_t dev_paddr = 0x43C00000;
};

#endif // __SIMIF_PTHREAD_H
