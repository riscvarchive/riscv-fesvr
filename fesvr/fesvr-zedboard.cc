// See LICENSE for license details.
#include "htif_zedboard.h"
#include <unistd.h>

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
  std::vector<std::string> args(argv + 1, argv + argc);
  htif_zedboard_t htif(args);

  bool memtest = false;
  int memtest_mb;

  int slowio = htif.read_cr(-1, 63);
  int divisor = slowio & 0xffff;
  int hold = (slowio >> 16) & 0xffff;

  for (std::vector<std::string>::const_iterator a = args.begin(); a != args.end(); ++a)
  {
    if (a->substr(0, 9) == "+memtest=")
      memtest = true, memtest_mb = std::atoi(a->substr(9).c_str());
    if (a->substr(0, 9) == "+divisor=")
      divisor = std::atoi(a->substr(9).c_str());
    if (a->substr(0, 6) == "+hold=")
      hold = std::atoi(a->substr(6).c_str());
  }

  htif.write_cr(-1, 63, divisor | (hold<<16));
  slowio = htif.read_cr(-1, 63);
  divisor = slowio & 0xffff;
  hold = (slowio >> 16) & 0xffff;

  float mhz = htif.get_host_clk_freq();

  printf("uncore slowio divisor=%d, hold=%d\n", slowio & 0xffff, (slowio >> 16) & 0xffff);
  printf("host_clk frequency = %0.2f MHz\n", mhz);
  printf("cpu_clk frequency  = %0.2f MHz\n", mhz * (divisor+1));
  if (memtest)
  {
    fprintf(stderr, "running memory test for %d MB...\n", memtest_mb);

    uint64_t nwords = memtest_mb*1024*1024/8;
    uint64_t* target_memory = new uint64_t[nwords]; // 1MB
    srand(time(NULL));
    for (uint64_t a=0; a<nwords; a++)
      target_memory[a] = random() << 32 | random();
    for (uint64_t a=0; a<nwords/8; a++) {
      htif.memif().write(a*64, 64, (uint8_t*)&target_memory[a*8]);
      fprintf(stderr, "\rwrote %016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx at 0x%08x",
        target_memory[a*8],
        target_memory[a*8+1],
        target_memory[a*8+2],
        target_memory[a*8+3],
        target_memory[a*8+4],
        target_memory[a*8+5],
        target_memory[a*8+6],
        target_memory[a*8+7],
        (uint32_t)a*64);
    }
    fprintf(stderr, "\n");

    int failcnt = 0;
    uint64_t chunk[8];
    for (uint64_t a=0; a<nwords/8; a++) {
      htif.memif().read(a*64, 64, (uint8_t*)chunk);
      fprintf(stderr, "\rread  %016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx at 0x%08x",
        chunk[0],
        chunk[1],
        chunk[2],
        chunk[3],
        chunk[4],
        chunk[5],
        chunk[6],
        chunk[7],
        (uint32_t)a*64);
      bool fail = false; 
      for (int l=0; l<8; l++) {
        uint64_t diff = chunk[l] ^ target_memory[a*8+l];
        if (diff) {
          if (!fail) {
            fprintf(stderr, "\n");
            fail = true;
          }
          fprintf(stderr, "mem diff at memory location 0x%08x, desired=%016llx, read=%016llx\n", (uint32_t)a*8+l, target_memory[a*8+l], chunk[l]);
        }
        failcnt += count_1bits(diff);
        failcnt += count_1bits(diff>>32);
      }
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "done memory test BER=%d/%d=%.9f...\n", failcnt, (uint32_t)nwords*8*8, (double)failcnt/(double(nwords*8*8)));
     
    delete [] target_memory;
  }

  printf("about to call htif.run()\n");
  return htif.run();
}
