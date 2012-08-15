#include "htif_isasim.h"
#include "htif_csim.h"
#include "htif_rs232.h"
#include "htif_eth.h"
#include "memif.h"
#include "elf.h"
#include "syscall.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <queue>
#include <string>

uint64_t mainvars[512];
size_t mainvars_sz;

void load_pk(memif_t* memif);
void load_program(const char* name, memif_t* memif);
int poll_tohost(htif_t* htif, int coreid, addr_t sig_addr, int sig_len);
void poll_devices(htif_t* htif);

int main(int argc, char** argv)
{
  int exit_code = 0;
  bool pkrun = true;
  bool testrun = false;
  htif_t* htif = NULL;
  int ncores = 1, i;
  int coreid = -1;
  addr_t sig_addr = 0;
  int sig_len = -1;
  bool assume0init = false;
  const char* csim_name = "./emulator";

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
    else if (s.substr(0,2) == "-p")
    {
      if (s.length() > 2)
        ncores = atoi(argv[i]+2);
    }
    else if (s.substr(0,2) == "-w")
    {
      if (s.length() > 2)
        coreid = atoi(argv[i]+2);
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
    else
      htif_args.push_back(argv[i]);
  }

  switch (simtype) // instantiate appropriate HTIF
  {
    case SIMTYPE_ISA: htif = new htif_isasim_t(ncores, htif_args); break;
    case SIMTYPE_RS232: htif = new htif_rs232_t(ncores, htif_args); break;
    case SIMTYPE_ETH: htif = new htif_eth_t(ncores, htif_args); break;
    case SIMTYPE_CSIM: htif = new htif_csim_t(ncores, csim_name, htif_args); break;
    default: abort();
  }
  htif->assume0init(assume0init);

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

  if (pkrun)
    load_pk(&htif->memif());
  else if (strcmp(target_argv[0], "none") != 0)
    load_program(target_argv[0], &htif->memif());

  if (coreid == -1)
  {
    for (int i = 0; i < ncores; i++)
      htif->start(i);
    coreid = 0;
  }
  else
  {
    htif->start(coreid);
  }

  if (testrun)
    exit_code = poll_tohost(htif, coreid, sig_addr, sig_len);
  else
    poll_devices(htif);

  htif->stop(coreid);
  delete htif;

  return exit_code;
}

void load_pk(memif_t* memif)
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
      load_elf(test_path.c_str(), memif);
      return;
    }
    p = next_p + 1;
  }
  while (next_p != NULL);

  fprintf(stderr, "could not locate riscv-pk in PATH\n");
  exit(-1);
}

void load_program(const char* name, memif_t* memif)
{
  if (access(name, F_OK) != 0)
  {
    fprintf(stderr, "could not open %s\n", name);
    exit(-1);
  }
  load_elf(name, memif);
}

int poll_tohost(htif_t* htif, int coreid, addr_t sig_addr, int sig_len)
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
      htif->memif().read(sig_addr, sig_len, signature);
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

    return 0;
  }

  fprintf(stderr, "*** FAILED *** (tohost = %ld)\n", (long)tohost);
  return -1;
}

struct core_status
{
  core_status() : poll_keyboard(0) {}
  std::queue<reg_t> fromhost;
  reg_t poll_keyboard;
};

void poll_devices(htif_t* htif)
{
  std::vector<core_status> status(htif->num_cores());

  while (true)
  {
    for (int coreid = 0; coreid < htif->num_cores(); coreid++)
    {
      core_status& s = status[coreid];
      reg_t tohost = htif->read_cr(coreid, 30);
      if (tohost == 0)
        continue;

      #define DEVICE(reg) (((reg) >> 56) & 0xFF)
      #define INTERRUPT(reg) (((reg) >> 48) & 0x80)
      #define COMMAND(reg) (((reg) >> 48) & 0x7F)
      #define PAYLOAD(reg) ((reg) & 0xFFFFFFFFFFFF)

      switch (DEVICE(tohost))
      {
        case 0: // proxy syscall
        {
          bool done = dispatch_syscall(htif, &htif->memif(), PAYLOAD(tohost));
          s.fromhost.push(1);
          if (done)
            return;
          break;
        }
        case 1: // console
        {
          switch (COMMAND(tohost))
          {
            case 0: // read
              if (s.poll_keyboard)
                continue;
              s.poll_keyboard = tohost;
              break;
            case 1: // write
            {
              unsigned char ch = PAYLOAD(tohost);
              if (ch == 0)
                return;
              assert(write(1, &ch, 1) == 1);
              break;
            }
          };
          break;
        }
      };
    }

    for (int coreid = 0; coreid < htif->num_cores(); coreid++)
    {
      core_status& s = status[coreid];
      if (s.poll_keyboard)
      {
        struct termios old_tios, new_tios;
        tcgetattr(0, &old_tios);
        new_tios = old_tios;
        new_tios.c_lflag &= ~(ICANON | ECHO);
        new_tios.c_cc[VMIN] = 0;
        tcsetattr(0, TCSANOW, &new_tios);
        unsigned char ch;
        if (read(0, &ch, 1) == 1)
        {
          s.fromhost.push(s.poll_keyboard | ch);
          s.poll_keyboard = 0;
        }
        else if (!INTERRUPT(s.poll_keyboard))
        {
          s.fromhost.push(s.poll_keyboard | 0x100);
          s.poll_keyboard = 0;
        }
        tcsetattr(0, TCSANOW, &old_tios);
      }

      if (!s.fromhost.empty())
      {
        reg_t value = s.fromhost.front();
        if (htif->write_cr(coreid, 31, value) == 0)
        {
          if (INTERRUPT(value))
            htif->write_cr(coreid, 9, 1);
          s.fromhost.pop();
        }
      }
    }
  }
}
