#ifndef __SIMIF_ZEDBORAD_H
#define __SIMIF_ZEDBORAD_H

#include "simif.h"

class simif_zedboard_t : public simif_t
{
  public:
    simif_zedboard_t(std::vector<std::string> args, bool log = false);
    ~simif_zedboard_t() { }
    virtual int run();

  private:
    virtual void poke(uint32_t value);
    virtual bool peek_ready();
    virtual uint32_t peek();

    bool poke_htif_ready();
    void poke_htif(uint32_t value);
    bool peek_htif_ready();
    uint32_t peek_htif();

    virtual void serve_htif(const size_t size);

    volatile uintptr_t* dev_vaddr;
    const static uintptr_t dev_paddr = 0x43C00000;
};

#endif // __SIMIF_ZEDBORAD_H
