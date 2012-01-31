#ifndef _APPSVR_SYSCALL_H
#define _APPSVR_SYSCALL_H

extern uint64_t mainvars[512];
extern size_t mainvars_sz;

#include "htif.h"
int dispatch_syscall(htif_t* htif, memif_t* memif, addr_t mm);

#endif
