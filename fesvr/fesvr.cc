#include "htif_isasim.h"
#include "htif_csim.h"
#include "htif_rs232.h"
#include "htif_eth.h"
#include "memif.h"
#include "elf.h"
#include "syscall.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <vector>
#include <string>

uint64_t mainvars[512];
size_t mainvars_sz;

unsigned int count_1bits(unsigned int x)
{
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = x + (x >> 8);
  x = x + (x >> 16);
  return x & 0x0000003F;
}

int main(int argc, char** argv)
{
  int exit_code = 0;
  bool pkrun = true;
  bool testrun = false;
  bool testmem = false;
  int testmem_mb = 0;
  bool write = false;
  bool read = false;
  htif_t* htif = NULL;
  int coreid = 0, i;
  addr_t sig_addr = 0;
  int sig_len = -1;
  bool assume0init = false;
  const char* csim_name = "./emulator";
  std::vector<char*>filenames;
  std::vector<size_t> start_addrs;
  int read_count = 0;
  int write_count = 0;

  enum {
    SIMTYPE_ISA,
    SIMTYPE_RS232,
    SIMTYPE_ETH,
    SIMTYPE_CSIM
  } simtype = SIMTYPE_ISA;

  std::vector<char*> htif_args;
  for (i = 1; i < argc && (argv[i][0] == '-' || argv[i][0] == '+'); i++)
  {
    std::string s = argv[i];
    if (s == "-testrun")
    {
      testrun = true;
      pkrun = false;
    }
    else if (s == "-nopk")
      pkrun = false;
    else if (s == "-rs232")
      simtype = SIMTYPE_RS232;
    else if (s == "-eth")
      simtype = SIMTYPE_ETH;
    else if (s.substr(0,2) == "-c")
    {
      simtype = SIMTYPE_CSIM;
      if (s.length() > 2)
        csim_name = argv[i]+2;
    }
    else if (s == "-testsig")
    {
      testrun = true;
      pkrun = false;
      assert(i + 2 < argc);

      sig_addr = (addr_t)atoll(argv[i+1]);
      sig_len = atoi(argv[i+2]);
      i += 2;
    }
    else if (s == "-assume0init")
      assume0init = true;
    else if (s == "-testmem")
    {
      testmem = true;
      testmem_mb = atoi(argv[i+1]);
      i += 1;
    }
    else if (s == "-write")
    {
      write = true;
      assert(i + 1 < argc);
      filenames.push_back(argv[i+1]);
      start_addrs.push_back(atoi(argv[i+2]));
      write_count++;
      i += 2;
    }
    else if (s == "-read")
    {
      read = true;
      assert(i + 1 < argc);
      filenames.push_back(argv[i+1]);
      start_addrs.push_back(atoi(argv[i+2]));
      read_count++;
      i += 2;
    }
    else if (s == "-rw")
    {
      read = true; write = true;
      assert(i + 1 < argc);
      filenames.push_back(argv[i+1]);
      start_addrs.push_back(atoi(argv[i+2]));
      write_count++;
      read_count++;
      i += 2;
    }
    else
      htif_args.push_back(argv[i]);
  }

  switch (simtype) // instantiate appropriate HTIF
  {
    case SIMTYPE_ISA: htif = new htif_isasim_t(htif_args); break;
    case SIMTYPE_RS232: htif = new htif_rs232_t(htif_args); break;
    case SIMTYPE_ETH: htif = new htif_eth_t(htif_args); break;
    case SIMTYPE_CSIM: htif = new htif_csim_t(csim_name, htif_args); break;
    default: abort();
  }
  htif->assume0init(assume0init);
  memif_t memif(htif);

  //while (true) {
  //  for (i=0; i<32; i++)
  //    htif->write_cr(0, i, 1);
  //  for (i=0; i<32; i++)
  //    printf("cr %d = %d\n", i, htif->read_cr(0, i));
  //}
  
  for (int j = 0; j < std::max(write_count, read_count); j++) {
    size_t fsz;
    char* buf;
    size_t start_addr = start_addrs[j];

    if (write || read)
    {
      // filesize in bytes
      FILE* file = fopen(filenames[j], "rb+");
      fseek(file, 0, SEEK_END);
      fsz = ftell(file);
      rewind(file);

      // read in the file
      buf = new char[fsz];
      fread(buf, fsz, 1, file);
      fclose(file);
    }

    if (write)
    {
      fprintf(stderr, "writing in data...\n");

      // write it out to the target mem
      for(size_t pos = 0; pos < fsz; pos+=64*1024) {
        memif.write(start_addr+pos, std::min(64*1024,(int)(fsz-pos)), (uint8_t*)&buf[pos]);
        fprintf(stderr, "\rwrote %d%%", pos*100/fsz);
      }
      fprintf(stderr, "\n");
    }

    if (read)
    {
      fprintf(stderr, "reading in data..\n");

      char* read_buf = new char[fsz];
      for (size_t pos = 0; pos < fsz; pos+=64*1024) {
        size_t len = std::min((size_t)(64*1024),fsz-pos);
        memif.read(start_addr+pos, len, (uint8_t*)(read_buf+pos));
        fprintf(stderr, "\rread %d%%", pos*100/fsz);

        for (size_t check = pos; check < pos + len; check++) {
          char diff = buf[check] ^ read_buf[check];
          if (diff) printf("Memory diff at 0x%08X. Expected: %02X. Got: %02X\n", start_addr+check, (unsigned char)buf[check], (unsigned char)read_buf[check]);
        }
      }

      fprintf(stderr, "\n");
      delete [] read_buf;
    }

    if (read || write) delete [] buf;
  }

  if (testmem)
  {
    fprintf(stderr, "running memory test for %d MB...\n", testmem_mb);

    uint64_t nwords = testmem_mb*1024*1024/8;
    uint64_t* target_memory = new uint64_t[nwords]; // 1MB
    srand(time(NULL));
    for (uint64_t a=0; a<nwords; a++)
      target_memory[a] = random() << 32 | random();
    for (uint64_t a=0; a<nwords/8; a++) {
      memif.write(a*64, 64, (uint8_t*)&target_memory[a*8]);
      fprintf(stderr, "\rwrote %016lx %016lx %016lx %016lx %016lx %016lx %016lx %016lx at 0x%08x",
        target_memory[a*8],
        target_memory[a*8+1],
        target_memory[a*8+2],
        target_memory[a*8+3],
        target_memory[a*8+4],
        target_memory[a*8+5],
        target_memory[a*8+6],
        target_memory[a*8+7],
        a*64);
    }
    fprintf(stderr, "\n");

    int failcnt = 0;
    uint64_t chunk[8];
    for (uint64_t a=0; a<nwords/8; a++) {
      memif.read(a*64, 64, (uint8_t*)chunk);
      fprintf(stderr, "\rread  %016lx %016lx %016lx %016lx %016lx %016lx %016lx %016lx at 0x%08x",
        chunk[0],
        chunk[1],
        chunk[2],
        chunk[3],
        chunk[4],
        chunk[5],
        chunk[6],
        chunk[7],
        a*64);
      bool fail = false; 
      for (int l=0; l<8; l++) {
        uint64_t diff = chunk[l] ^ target_memory[a*8+l];
        if (diff) {
          if (!fail) {
            fprintf(stderr, "\n");
            fail = true;
          }
          fprintf(stderr, "mem diff at memory location 0x%08x, desired=%016lx, read=%016lx\n", a*8+l, target_memory[a*8+l], chunk[l]);
        }
        failcnt += count_1bits(diff);
        failcnt += count_1bits(diff>>32);
      }
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "done memory test BER=%d/%d=%.9f...\n", failcnt, nwords*8*8, (double)failcnt/(double(nwords*8*8)));
     
    delete [] target_memory;
  }

  if (i == argc) // make sure the user specified a target program
  {
    fprintf(stderr, "usage: %s <binary> [args]\n", argv[0]);
    exit(-1);
  }

  char** target_argv = argv + i;
  int target_argc = argc - i;
  mainvars[0] = target_argc;
  mainvars[target_argc+1] = 0; // argv[argc] = NULL
  mainvars[target_argc+2] = 0; // envp[0] = NULL
  mainvars_sz = (target_argc+3) * sizeof(mainvars[0]);
  for (i = 0; i < target_argc; i++)
  {
    size_t len = strlen(target_argv[i]) + 1;
    mainvars[i+1] = mainvars_sz;
    memcpy((uint8_t*)mainvars + mainvars_sz, target_argv[i], len);
    mainvars_sz += len;
  }

  if (pkrun) // locate and load the proxy kernel, riscv-pk
  {
    const char* path = getenv("PATH");
    assert(path);
    char* s = strdup(path);
    char* p = s, *next_p;
    bool found = false;

    do
    {
      if ((next_p = strchr(p, ':')) != NULL)
        *next_p = '\0';
      std::string test_path = std::string(p) + "/riscv-pk";
      if (access(test_path.c_str(), F_OK) == 0)
      {
        load_elf(test_path.c_str(), &memif);
        found = true;
      }
      p = next_p + 1;
    }
    while (next_p != NULL && !found);

    if (!found)
    {
      fprintf(stderr, "could not locate riscv-pk in PATH\n");
      exit(-1);
    }
  }
  else if (strcmp(target_argv[0], "none") != 0)
  {
    if (access(target_argv[0], F_OK) != 0)
    {
      fprintf(stderr, "could not open %s\n", target_argv[0]);
      exit(-1);
    }
    load_elf(target_argv[0], &memif);
  }

  htif->start(coreid);

  if (testrun)
  {
    reg_t tohost;
    while ((tohost = htif->read_cr(coreid, 30)) == 0);
    if (tohost == 1)
    {
      if (sig_len != -1)
      {
        int chunk_size = 16;
        assert(sig_addr % chunk_size == 0);
        int sig_len_aligned = (sig_len + chunk_size - 1)/chunk_size*chunk_size;

        uint8_t* signature = new uint8_t[sig_len_aligned];
        memif.read(sig_addr, sig_len, signature);
        for (int i = sig_len; i < sig_len_aligned; i++)
          signature[i] = 0;

        for (int i = 0; i < sig_len_aligned; i += chunk_size)
        {
          for (int j = chunk_size - 1; j >= 0; j--)
            printf("%02x", signature[i+j]);
          printf("\n");
        }
        delete [] signature;
      }
      else
        fprintf(stderr, "*** PASSED ***\n");
    }
    else
    {
      fprintf(stderr, "*** FAILED *** (tohost = %ld)\n", (long)tohost);
      exit_code = -1;
    }
  }
  else
  {
    bool reset_termios = false;
    struct termios old_tios, new_tios;
    if (!pkrun && isatty(0))
    {
      // use non-canonical-mode input if not using the proxy kernel
      reset_termios = true;
      assert(tcgetattr(0, &old_tios) == 0);
      new_tios = old_tios;
      new_tios.c_lflag &= ~(ICANON | ECHO);
      new_tios.c_cc[VMIN] = 0;
      assert(tcsetattr(0, TCSANOW, &new_tios) == 0);
    }

    while (true)
    {
      reg_t tohost;
      while ((tohost = htif->read_cr(coreid, 30)) == 0);
      if (dispatch_syscall(htif, &memif, tohost))
        break;
      htif->write_cr(coreid, 31, 1);
    }

    if (reset_termios)
      tcsetattr(0, TCSANOW, &old_tios);
  }

  htif->stop(coreid);
  delete htif;

  return exit_code;
}
