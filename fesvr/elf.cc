#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>
#include "common.h"
#include "memif.h"

void load_elf(const char* fn, memif_t* memif)
{
  int fd = open(fn, O_RDONLY);
  demand(fd != -1, "couldn't open %s", fn);

  struct stat s;
  demand(fstat(fd,&s) != -1, "couldn't stat %s", fn);

  size_t size = s.st_size;

  char* buf = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  demand(buf != MAP_FAILED, "couldn't mmap %s", fn);

  close(fd);

  demand(size >= sizeof(Elf64_Ehdr), "truncated ELF!");
  const Elf64_Ehdr* eh64 = (const Elf64_Ehdr*)buf;
  demand(strncmp((const char*)eh64->e_ident,ELFMAG,strlen(ELFMAG)) == 0, "bad ELF!");

  #define LOAD_ELF do { \
    eh = (typeof(eh))buf; \
    demand(size >= eh->e_phoff + eh->e_phnum*sizeof(*ph), "bad ELF!"); \
    ph = (typeof(ph))(buf+eh->e_phoff); \
    for(int i = 0; i < eh->e_phnum; i++, ph++) { \
      if(ph->p_type == SHT_PROGBITS && ph->p_memsz) { \
        demand(size >= ph->p_offset + ph->p_filesz, "bad ELF!"); \
        memif->write(ph->p_vaddr, ph->p_filesz, (uint8_t*)buf + ph->p_offset); \
        memif->write(ph->p_vaddr + ph->p_filesz, ph->p_memsz - ph->p_filesz, NULL); \
      } \
    } \
  } while(0)

  if(eh64->e_ident[EI_CLASS] == ELFCLASS32)
  {
    Elf32_Ehdr* eh;
    Elf32_Phdr* ph;
    LOAD_ELF;
  }
  else
  {
    demand(eh64->e_ident[EI_CLASS] == ELFCLASS64, "bad ELF!");
    Elf64_Ehdr* eh;
    Elf64_Phdr* ph;
    LOAD_ELF;
  }

  munmap(buf, size);
}
