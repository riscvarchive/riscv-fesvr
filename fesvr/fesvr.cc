#include "htif_isasim.h"
#include "htif_csim.h"
#include "htif_rtlsim.h"
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
#include <vector>

uint64_t mainvars[512];
size_t mainvars_sz;


int main(int argc, char** argv)
{
  int exit_code = 0;
  bool pkrun = true;
  bool testrun = false;
  htif_t* htif = NULL;
  int coreid = 0, i;
  addr_t sig_addr = 0;
  int sig_len = -1;

  enum {
    SIMTYPE_ISA,
    SIMTYPE_RTL,
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
    else if (s == "-rtl")
      simtype = SIMTYPE_RTL;
    else if (s == "-gl")
    {
      simtype = SIMTYPE_RTL;
      htif_args.push_back(const_cast<char*>("-ucli"));
      htif_args.push_back(const_cast<char*>("-do"));
      htif_args.push_back(const_cast<char*>("run.tcl"));
    }
    else if (s == "-rs232")
      simtype = SIMTYPE_RS232;
    else if (s == "-eth")
      simtype = SIMTYPE_ETH;
    else if (s == "-c")
      simtype = SIMTYPE_CSIM;
    else if (s == "-testsig")
    {
      testrun = true;
      pkrun = false;
      assert(i + 2 < argc);

      sig_addr = (addr_t)atoll(argv[i+1]);
      sig_len = atoi(argv[i+2]);
      i += 2;
    }
    else
      htif_args.push_back(argv[i]);
  }

  switch (simtype) // instantiate appropriate HTIF
  {
    case SIMTYPE_ISA: htif = new htif_isasim_t(htif_args); break;
    case SIMTYPE_RTL: htif = new htif_rtlsim_t(htif_args); break;
    case SIMTYPE_RS232: htif = new htif_rs232_t(htif_args); break;
    case SIMTYPE_ETH: htif = new htif_eth_t(htif_args); break;
    case SIMTYPE_CSIM: htif = new htif_csim_t(htif_args); break;
    default: abort();
  }
  memif_t memif(htif);

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

    do
    {
      if ((next_p = strchr(p, ':')) != NULL)
        *next_p = '\0';
      std::string test_path = std::string(p) + "/riscv-pk";
      if (access(test_path.c_str(), F_OK) == 0)
      {
        load_elf(test_path.c_str(), &memif);
        break;
      }
    }
    while((p = next_p) != NULL);
  }
  else // merely load the specified target program
    load_elf(target_argv[0], &memif);

  htif->start(coreid);

  if (testrun)
  {
    reg_t tohost;
    while ((tohost = htif->read_cr(coreid, 16)) == 0);
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
        printf("*** PASSED ***\n");
    }
    else
    {
      printf("*** FAILED *** (tohost = %ld)\n", (long)tohost);
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
      while ((tohost = htif->read_cr(coreid, 16)) == 0);
      if (dispatch_syscall(htif, &memif, tohost))
        break;
      htif->write_cr(coreid, 17, 1);
    }

    if (reset_termios)
      tcsetattr(0, TCSANOW, &old_tios);
  }

  htif->stop(coreid);
  delete htif;

  return exit_code;
}
