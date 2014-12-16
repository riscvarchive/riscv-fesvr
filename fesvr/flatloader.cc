// See LICENSE for license details.

#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<assert.h>
#include<unistd.h>
#include"memif.h"

void load_flat(const char *fn, memif_t* memif)
{
  int fd = open(fn, O_RDONLY);
  struct stat s;
  assert(fd != -1);
  assert(fstat(fd, &s) != -1);
  size_t size = s.st_size;

  void *buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(buf != MAP_FAILED);
  close(fd);

  memif->write(0x2000, size, buf);

  munmap(buf, size);
}
