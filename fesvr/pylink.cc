#include "pylink.h"
#include "htif_isasim.h"
#include "htif_rtlsim.h"
#include "htif_csim.h"
#include "htif_rs232.h"
#include "htif_eth.h"
#include "memif.h"
#include "elf.h"
#include "syscall.h"

char mainvars[0x1000];
long* mainvars_longp = (long*)mainvars;
size_t mainvars_sz;

void* new_htif_isasim(int fdin, int fdout)
{
  return new htif_isasim_t(fdin, fdout);
}

void* new_htif_rtlsim(int fdin, int fdout)
{
  return new htif_rtlsim_t(fdin, fdout);
}

void* new_htif_csim(int fdin, int fdout)
{
  return new htif_csim_t(fdin, fdout);
}

void* new_htif_rs232(const char* tty)
{
  return new htif_rs232_t(tty);
}

void* new_htif_eth(const char* tty, int sim)
{
  return new htif_eth_t(tty, bool(sim));
}

void* new_memif(void* htif)
{
  return new memif_t((htif_t*)htif);
}

void load_elf(const char* fn, void* memif)
{
  load_elf(fn, (memif_t*)memif);
}

void htif_start(void* htif, int coreid)
{
  ((htif_t*)htif)->start(coreid);
}

void htif_stop(void* htif, int coreid)
{
  ((htif_t*)htif)->stop(coreid);
}

reg_t htif_read_cr_until_change(void* htif, int coreid, int regnum)
{
  reg_t val;

  do
  {
    val = ((htif_t*)htif)->read_cr(coreid, regnum);
  } while (val == 0);

  return val;
}

reg_t htif_read_cr(void* htif, int coreid, int regnum)
{
  return ((htif_t*)htif)->read_cr(coreid, regnum);
}

void htif_write_cr(void* htif, int coreid, int regnum, reg_t val)
{
  ((htif_t*)htif)->write_cr(coreid, regnum, val);
}

void frontend_syscall(void* htif, void* memif, addr_t mm)
{
  dispatch_syscall((htif_t*)htif, (memif_t*)memif, mm);
}

void mainvars_argc(int argc)
{
  mainvars_longp[0] = argc;
  mainvars_longp[argc+1] = 0; // argv[argc] = NULL
  mainvars_longp[argc+2] = 0; // envp = NULL
  mainvars_sz = (argc+3) * sizeof(long);
}

void mainvars_argv(int idx, int len, char* arg)
{
  len++; // count the null terminator as well
  mainvars_longp[idx+1] = mainvars_sz;
  memcpy(&mainvars[mainvars_sz], arg, len);
  mainvars_sz += len;
}
