#ifndef __HTIF_CSIM_H
#define __HTIF_CSIM_H

#include "htif.h"
#include <vector>

class htif_csim_t : public htif_t
{
 public:
  htif_csim_t(int ncores, const char* progname, std::vector<char*> args);
  ~htif_csim_t();

  uint32_t mem_mb() { return 512; }

 protected:
  ssize_t read(void* buf, size_t max_size)
  {
    int ret = ::read(fdin, buf, max_size+14);
    if (ret > 14)
        memcpy(buf,(uint8_t *)buf+14,ret-14);
    //fprintf(stderr, "read (%d): ", ret - 14);
    //for (int i = 0; i < ret - 14; i++)
    //    fprintf(stderr, "%x ", *((uint8_t *)buf+i));
    //fprintf(stderr, "\n\n");
    return ret - 14;
  }

  ssize_t write(const void* buf, size_t size)
  {
    uint8_t *nBuf = (uint8_t *) malloc(size+14);
    nBuf[0] = 0x0;
    nBuf[1] = 0x50;
    nBuf[2] = 0xc2;
    nBuf[3] = 0x95;
    nBuf[4] = 0xe8;
    nBuf[5] = 0x6c;
    nBuf[6] = 0xde;
    nBuf[7] = 0xad;
    nBuf[8] = 0xbe;
    nBuf[9] = 0xef;
    nBuf[13] = size;
    memcpy(nBuf+14, buf, size);
    //fprintf(stderr, "write: ");
    //for (int i = 0; i < size; i++)
    //    fprintf(stderr, "%x ", nBuf[i+14]);
    //fprintf(stderr, "\n");
    return ::write(fdout, nBuf, size+14);
  }

  size_t chunk_max_size() { return 64; }
  size_t chunk_align() { return 64; }

 private:
  int pid;
  int fdin;
  int fdout;
};

#endif // __HTIF_CSIM_H
