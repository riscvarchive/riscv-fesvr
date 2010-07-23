#ifndef _APPSVR_SYSCALL_H
#define _APPSVR_SYSCALL_H

#include "htif.h"
void dispatch_syscall(htif_t* htif, memif_t* memif, addr_t mm);

#endif
