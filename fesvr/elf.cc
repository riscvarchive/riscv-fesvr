#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include "elf.h"
#include "memif.h"
#include <stdlib.h>

void load_elf(const char* fn, memif_t* memif)
{
// memory test
#if 0
  const int N = 4096;
  uint32_t x[N], y[N];
  for(int i = 0; i < N; i++)
    x[i] = rand();

  memif->write(0, sizeof(x), NULL);
  memif->write(0, sizeof(x), (uint8_t*)x);
  memif->read(0, sizeof(x), (uint8_t*)y);

  assert(memcmp(x, y, sizeof(x)) == 0);
#endif

  int fd = open(fn, O_RDONLY);
  assert(fd != -1);

  struct stat s;
  assert(fstat(fd,&s) != -1);

  size_t size = s.st_size;

  char* buf = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(buf != MAP_FAILED);

  close(fd);

  assert(size >= sizeof(Elf64_Ehdr));
  const Elf64_Ehdr* eh64 = (const Elf64_Ehdr*)buf;
  assert(strncmp((const char*)eh64->e_ident,ELFMAG,strlen(ELFMAG)) == 0);

  #define LOAD_ELF do { \
    eh = (typeof(eh))buf; \
    assert(size >= eh->e_phoff + eh->e_phnum*sizeof(*ph)); \
    ph = (typeof(ph))(buf+eh->e_phoff); \
    for(int i = 0; i < eh->e_phnum; i++, ph++) { \
      if(ph->p_type == SHT_PROGBITS && ph->p_memsz) { \
        assert(size >= ph->p_offset + ph->p_filesz); \
        memif->write(ph->p_paddr, ph->p_filesz, (uint8_t*)buf + ph->p_offset); \
        memif->write(ph->p_paddr + ph->p_filesz, ph->p_memsz - ph->p_filesz, NULL); \
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
    assert(eh64->e_ident[EI_CLASS] == ELFCLASS64);
    Elf64_Ehdr* eh;
    Elf64_Phdr* ph;
    LOAD_ELF;
  }

  munmap(buf, size);
}
