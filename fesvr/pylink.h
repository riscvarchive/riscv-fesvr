#ifndef _PYLINK_H
#define _PYLINK_H

#include "htif.h"

#ifdef __cplusplus
extern "C"
{
#endif

// WARNING: if you change any of these prototypes, change fesvr_pylink.py, too!

void* new_htif_isasim(int fdin, int fdout);
void* new_htif_rtlsim(int fdin, int fdout);
void* new_htif_csim(int fdin, int fdout);
void* new_htif_rs232(const char* tty);
void* new_htif_eth(const char* tty, int sim);
void* new_memif(void* htif);

void load_elf(const char* fn, void* memif);

void htif_start(void* htif, int coreid);
void htif_stop(void* htif, int coreid);
reg_t htif_read_cr_until_change(void* htif, int coreid, int regnum);
reg_t htif_read_cr(void* htif, int coreid, int regnum);
void htif_write_cr(void* htif, int coreid, int regnum, reg_t val);

int frontend_syscall(void* htif, void* memif, addr_t mm);
void mainvars_argc(int argc);
void mainvars_argv(int idx, int len, char* arg);

#ifdef __cplusplus
}
#endif

#endif

