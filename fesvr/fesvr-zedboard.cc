// See LICENSE for license details.
#include "htif_zedboard.h"
#include "bist.h"
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
  bist_t bist(&htif,args);

  bool memtest = false;
  int memtest_mb = 0;

  int divisor = 31;
  int hold = 2;
  int bisten = 0;
  float vdd_backup = 1.0;
  float freq = 50e6;
  int bist_clksel = 0;
  int uncore_clksel = 1;
  int cassia_clksel = 0;
  int core_clksel = 0;
  unsigned int saen_width_ctrl=1;
  unsigned int write_delay_ctrl=0;
  unsigned int write_timing_sel=0;
  unsigned int saen_sel=0;
  unsigned int use_sa=0;
  unsigned int use_fbb=0;
  unsigned int n_vref_ctrl=3;
  unsigned int saen_delay_ctrl=0;
  unsigned int bl_boost_ctrl=0;

  for (std::vector<std::string>::const_iterator a = args.begin(); a != args.end(); ++a)
  {
    if (a->substr(0, 9) == "+memtest=")
      memtest = true, memtest_mb = std::atoi(a->substr(9).c_str());
    if (a->substr(0, 9) == "+divisor=")
      divisor = std::atoi(a->substr(9).c_str());
    if (a->substr(0, 6) == "+hold=")
      hold = std::atoi(a->substr(6).c_str());
    if (a->substr(0, 6) == "+freq=")
      freq = std::atof(a->substr(6).c_str());
    if (a->substr(0, 6) == "+bist=")
      bisten = 1;
    if (a->substr(0, 5) == "+vdd=")
      vdd_backup = std::atof(a->substr(5).c_str());
    if (a->substr(0, 13) == "+bist_clksel=")
      bist_clksel = std::atof(a->substr(13).c_str());
    if (a->substr(0, 13) == "+core_clksel=")
      core_clksel = std::atof(a->substr(13).c_str());
    if (a->substr(0, 15) == "+cassia_clksel=")
      cassia_clksel = std::atof(a->substr(15).c_str());
    if (a->substr(0, 15) == "+uncore_clksel=")
      uncore_clksel = std::atof(a->substr(15).c_str());
    if (a->substr(0, 17) == "+saen_width_ctrl=")
      saen_width_ctrl = std::atoi(a->substr(17).c_str());
    if (a->substr(0, 18) == "+write_delay_ctrl=")
      write_delay_ctrl = std::atoi(a->substr(18).c_str());
    if (a->substr(0, 18) == "+write_timing_sel=")
      write_timing_sel = std::atoi(a->substr(18).c_str());
    if (a->substr(0, 10) == "+saen_sel=")
      saen_sel = std::atoi(a->substr(10).c_str());
    if (a->substr(0, 8) == "+use_sa=")
      use_sa = std::atoi(a->substr(8).c_str());
    if (a->substr(0, 9) == "+use_fbb=")
      use_fbb = std::atoi(a->substr(9).c_str());
    if (a->substr(0, 13) == "+n_vref_ctrl=")
      n_vref_ctrl = std::atoi(a->substr(13).c_str());
    if (a->substr(0, 17) == "+saen_delay_ctrl=")
      saen_delay_ctrl = std::atoi(a->substr(17).c_str());
    if (a->substr(0, 15) == "+bl_boost_ctrl=")
      bl_boost_ctrl = std::atoi(a->substr(15).c_str());
  }

  htif.set_i2c_divider(7);
  htif.set_voltage(I2C_R3_VDDHI, 1.8);
  if(!bisten){
    printf("Setting vdd to: %f\n",vdd_backup);
    htif.set_voltage(I2C_R3_VDDLO, vdd_backup);
  }
  htif.set_voltage(I2C_R3_VDD18, 1.8);
  htif.set_voltage(I2C_R3_VDD10, 1.0);
  htif.set_reference_voltage(I2C_R3_VDDS, 0);
  htif.set_reference_voltage(I2C_R3_GNDS, 0);
  htif.set_reference_voltage(I2C_R3_VREF, 0.75);
  htif.set_reference_voltage(I2C_R3_VDD_REPLICA, 0.2);
  htif.read_voltage(I2C_R3_VDDHI);
  htif.read_voltage(I2C_R3_VDDLO);
  htif.read_voltage(I2C_R3_VDD18);
  htif.read_voltage(I2C_R3_VDD10);
  htif.write_clock(freq);
  htif.set_clksel(uncore_clksel);

  htif.write_cr(-1, 63, divisor | (hold<<16));
  int slowio = htif.read_cr(-1, 63);
  divisor = slowio & 0xffff;
  hold = (slowio >> 16) & 0xffff;
  float mhz = htif.get_host_clk_freq();
  printf("uncore slowio divisor=%d, hold=%d\n", slowio & 0xffff, (slowio >> 16) & 0xffff);
  printf("host_clk frequency = %0.2f MHz\n", mhz);
  printf("cpu_clk frequency  = %0.2f MHz\n", mhz * (divisor+1));

  htif.cassia_init();
  htif.clock_init(core_clksel, cassia_clksel, bist_clksel);
  htif.bz_sram_init(saen_width_ctrl, write_delay_ctrl, write_timing_sel, saen_sel, use_sa,use_fbb,n_vref_ctrl,saen_delay_ctrl,bl_boost_ctrl);
  htif.st_sram_init();
  htif.dcdc_init();

  if(bisten)
  {
    bist.run_bist();
  }
  
  printf("cores = %d\n", (int)htif.read_cr(-1, 0));
  printf("mb = %d\n", (int)htif.read_cr(-1, 1));
  printf("reset = %d\n", (int)htif.read_cr(0, 29));
  htif.write_cr(0, 13, 0xdeadbeef);
  printf("k0 = %08lx\n", (long unsigned int)htif.read_cr(0, 13));

  if (memtest)
  {
    fprintf(stderr, "running memory test for %d MB...\n", memtest_mb);

    uint64_t nwords = memtest_mb*1024*1024/8;
    uint64_t* target_memory = new uint64_t[nwords]; // 1MB
    srand(time(NULL));
    for (uint64_t a=0; a<nwords; a++)
      target_memory[a] = ((uint64_t) random()) << 32 | random();
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

  if(!bisten){
  printf("about to call htif.run()\n");
  return htif.run();
  }
}
