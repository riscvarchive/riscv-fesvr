#include "htif_isasim.h"
#include "htif_rtlsim.h"
#include "htif_rs232.h"
#include "memif.h"
#include "elf.h"
#include "syscall.h"

char mainvars[0x1000];
long* mainvars_longp = (long*)mainvars;
size_t mainvars_sz;

extern "C"
{

char* new_htif_isasim(int fdin, int fdout)
{
  htif_isasim_t* htif = new htif_isasim_t(fdin, fdout);

  return (char*)htif;
}

char* new_htif_rtlsim(int fdin, int fdout)
{
  htif_rtlsim_t* htif = new htif_rtlsim_t(fdin, fdout);

  return (char*)htif;
}

char* new_htif_rs232(const char* tty)
{
  htif_rs232_t* htif = new htif_rs232_t(tty);

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

void mainvars_argc(int argc)
{
  mainvars_longp[0] = argc;
  mainvars_longp[argc+1] = 0; // argv[argc] = NULL
  mainvars_longp[argc+2] = 0; // envp = NULL
  mainvars_sz = (argc+3) * sizeof(long);
}

void mainvars_argv(int idx, int len, char* arg)
{
  len++; // count the NULL pointer as well
  mainvars_longp[idx+1] = mainvars_sz;
  memcpy(&mainvars[mainvars_sz], arg, len);
  mainvars_sz += len;
}

}
