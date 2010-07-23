#ifndef __ELF_H
#define __ELF_H

#include_next <elf.h>
#include "memif.h"

void load_elf(const char* fn, memif_t* memif);

#endif // __ELF_H
