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
  const Elf64_Ehdr* eh = (const Elf64_Ehdr*)buf;

  const uint32_t* magic = (const uint32_t*)eh->e_ident;
  demand(*magic == *(const uint32_t*)ELFMAG, "not a host-endian ELF!");
  demand(size >= eh->e_phoff + eh->e_phnum*sizeof(Elf64_Phdr), "bad ELF!");
  const Elf64_Phdr* phs = (const Elf64_Phdr*)(buf+eh->e_phoff);

  for(int i = 0; i < eh->e_phnum; i++)
  {
    const Elf64_Phdr* ph = &phs[i];
    if(ph->p_type == SHT_PROGBITS && ph->p_memsz)
    {
      demand(size >= ph->p_offset + ph->p_filesz, "truncated ELF!");
      demand(ph->p_memsz >= ph->p_filesz, "bad ELF!");

      memif->write(ph->p_vaddr, ph->p_filesz, (uint8_t*)buf + ph->p_offset);
      memif->write(ph->p_vaddr + ph->p_filesz, ph->p_memsz - ph->p_filesz, NULL);
    }
  }

  munmap(buf, size);
}
