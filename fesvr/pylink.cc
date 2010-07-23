#include "htif_isasim.h"
#include "memif.h"
#include "elf.h"
#include "syscall.h"

extern "C"
{

char* new_htif_isasim(int fdin, int fdout)
{
  htif_isasim_t* htif = new htif_isasim_t(fdin, fdout);

  return (char*)htif;
}

char* new_memif(char* htif)
{
  memif_t* memif = new memif_t((htif_t*)htif);

  return (char*)memif;
}

void load_elf(const char* fn, char* memif)
{
  load_elf(fn, (memif_t*)memif);
}

void htif_start(char* htif)
{
  ((htif_t*)htif)->start();
}

void htif_stop(char* htif)
{
  ((htif_t*)htif)->stop();
}

reg_t htif_read_cr(char* htif, int coreid, int regnum)
{
  return ((htif_t*)htif)->read_cr(coreid, regnum);
}

void htif_write_cr(char* htif, int coreid, int regnum, reg_t val)
{
  ((htif_t*)htif)->write_cr(coreid, regnum, val);
}

void frontend_syscall(char* htif, char* memif, addr_t mm)
{
  dispatch_syscall((htif_t*)htif, (memif_t*)memif, mm);
}

}
