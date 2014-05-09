#include "htif_zedboard.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "fout.h" // Lookup table for clock gen

#define read_reg(r) (dev_vaddr[r])
#define write_reg(r, v) (dev_vaddr[r] = v)

htif_zedboard_t::htif_zedboard_t(const std::vector<std::string>& args)
  : htif_t(args)
{
  int fd = open("/dev/mem", O_RDWR|O_SYNC);
  assert(fd != -1);
  dev_vaddr = (uintptr_t*)mmap(0, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, dev_paddr);
  assert(dev_vaddr != MAP_FAILED);

  printf("about to toggle cpu reset pin\n");
  write_reg(31, 1);
  usleep(10000);
  write_reg(31, 0);
  printf("done\n");
}

htif_zedboard_t::~htif_zedboard_t()
{
}

void htif_zedboard_t::reset_internal()
{
  write_reg(31, 2);
  usleep(10000);
  write_reg(31, 0);
}


float htif_zedboard_t::get_host_clk_freq()
{
  uintptr_t f1, f2;
  // read multiple times to ensure accuracy
  do {
    f1 = read_reg(2);
    usleep(10000);
    f2 = read_reg(2);
  } while (abs(f1-f2) > 500);

  float freq = f1 / 1E4;
  return freq;
}

void htif_zedboard_t::st_sram_init()
{
  write_cr(-1, 62, 1);
  write_cr(-1, 62, 0);
  write_cr(-1, 62, 1);
}

void htif_zedboard_t::bz_sram_init(unsigned int saen_width_ctrl, unsigned int write_delay_ctrl, unsigned int write_timing_sel, unsigned int saen_sel, unsigned int use_sa,unsigned int use_fbb,unsigned int n_vref_ctrl,unsigned int saen_delay_ctrl,unsigned int bl_boost_ctrl)
{
  uint16_t bzconf = 0;
  bzconf = bzconf | (uint16_t) ((saen_width_ctrl & 0x3) << 14);
  bzconf = bzconf | (uint16_t) ((write_delay_ctrl & 0x3) << 12);
  bzconf = bzconf | (uint16_t) ((write_timing_sel & 0x1) << 11);
  bzconf = bzconf | (uint16_t) ((saen_sel & 0x3) << 9);
  bzconf = bzconf | (uint16_t) ((use_sa & 0x1) << 8);
  bzconf = bzconf | (uint16_t) ((use_fbb & 0x1) << 7);
  bzconf = bzconf | (uint16_t) ((n_vref_ctrl & 0x3) << 5);
  bzconf = bzconf | (uint16_t) ((saen_delay_ctrl & 0x3) << 3);
  bzconf = bzconf | (uint16_t) ((bl_boost_ctrl & 0x7));
  write_cr(-1, 61, bzconf);
  printf("bzconf: %016x\n",bzconf);
}


void htif_zedboard_t::dcdc_init(unsigned int dcdc_conf)
{
  write_cr(-1, 15, 2);
  write_cr(-1, 16, 0xffffff);

  write_cr(-1, 8, 0);
  write_cr(-1, 9, 0);
  write_cr(-1, 10, 0);
  write_cr(-1, 11, 1);
  write_cr(-1, 12, 3);
  write_cr(-1, 13, 1);
  write_cr(-1, 14, 2);

  write_cr(-1, 11, 0);
  write_cr(-1, 8, 1);
  write_cr(-1, 10, 1);
  write_cr(-1, 11, 1);
  write_cr(-1, 57, 1);
  if (dcdc_conf == 3)
  {
    write_cr(-1, 8, 0); // 0.9V
  }
  else if (dcdc_conf == 2)
  {
    write_cr(-1, 8, 2); // 0.67V
  }
  else if (dcdc_conf == 1)
  {
    write_cr(-1, 8, 6); // 0.5V
  }
}

void htif_zedboard_t::clock_init(int core_clksel, int cassia_clksel, int bist_clksel)
{
  write_cr(-1, 52, core_clksel); // core_clksel
  write_cr(-1, 53, 0); // cassia_clksel
  write_cr(-1, 54, 0); // dcdc_clksel
  write_cr(-1, 55, 0); // dcdcslow_clksel
  write_cr(-1, 56, bist_clksel); // bist_clksel
  write_cr(-1, 57, 0); // dcdc_counter_clksel
}

#define bit(in, pos) \
  (((in)>>pos)&0x1)

static unsigned five_bit_swap(unsigned in)
{
  return (bit(in,0)<<4) | (bit(in,1)<<3) | (bit(in,2)<<2) | (bit(in,3)<<1) | (bit(in,4)<<0);
}

void htif_zedboard_t::cassia_init(unsigned int s1, unsigned int s2)
{
  //assign dll_en = cassia_ctrl[0];
  //assign ctrl_en = cassia_ctrl[1];
  //assign clk_en = cassia_ctrl[2];
  //assign wd_en = cassia_ctrl[3];
  //assign ctrl_s1 = cassia_ctrl[8:4];
  //assign ctrl_s2 = cassia_ctrl[13:9];

  int cassia_seed = (five_bit_swap(s2&0x1f)<<9)|(five_bit_swap(s1&0x1f)<<4)|(1<<3)|(1<<2);
  write_cr(-1, 48, 0);
  write_cr(-1, 48, cassia_seed);
  cassia_seed = cassia_seed | 1;
  write_cr(-1, 48, cassia_seed);
  usleep(10000);
  cassia_seed = cassia_seed | (1<<1);
  write_cr(-1, 48, cassia_seed);
}

void htif_zedboard_t::write_i2c_reg(short supply_name, short reg_addr, short num_bytes, short wdata)
{
  write_reg(I2C_SLAVE_ADDR, supply_name << 1 | 0);
  write_reg(I2C_REG_ADDR, num_bytes << 8 | (reg_addr & 0xFF));
  write_reg(I2C_WDATA, wdata);
  write_reg(I2C_TOGGLE, 1); 
  printf("slave_addr: %d, reg_addr: %d, wdata: %d\n",supply_name, reg_addr & 0xFF,wdata);
}

short htif_zedboard_t::read_i2c_reg(short supply_name, short reg_addr, short num_bytes)
{
  short rdata;

  write_reg(I2C_SLAVE_ADDR, supply_name << 1 | 1);
  write_reg(I2C_REG_ADDR, num_bytes << 8 | (reg_addr & 0xFF));
  write_reg(I2C_TOGGLE, 1); 
  rdata = read_reg(I2C_RDATA);
  printf("rdata: %d\n",(int) rdata);
  return rdata;
}

void htif_zedboard_t::set_i2c_divider(short divider)
{
  write_reg(I2C_DIVISOR, 1 << divider); // divisor
}

int freq_compare(const void *c1, const void *c2)
{
  return ((const struct clk_lookup*) c1)->fout - ((const struct clk_lookup*) c2)->fout;
  //return 1;
}

void htif_zedboard_t::set_clksel(int sel)
{
  write_reg(CLK_SEL_ADDR, sel);
}

void htif_zedboard_t::write_clock(float freq)
{
  short new_r135;
  short old_r135;
  unsigned count = sizeof(clk_freqs) / sizeof(struct clk_lookup);
  struct clk_lookup *result;
  uint64_t rfreq_hex;
  uint64_t whole,frac;

  // Search for closest matching frequency
  unsigned idx;
  for (idx=0; idx<count; idx++) {
    if (clk_freqs[idx].fout > freq) {
      break;
    }
  }
  if (idx != 0) {
    idx--;
  }
  result = &clk_freqs[idx];
  printf("found freq:%f\n",result->fout);

  whole = ((unsigned int) result->rfreq);
  frac = (unsigned int) ((result->rfreq-(float) whole)*(1<<28));
  rfreq_hex = ((whole << 28) & 0x3FF0000000UL) | (frac & 0xFFFFFFFUL);

  // Freeze DCO
  write_i2c_reg(I2C_R3_CLOCK, 137, 1, 0X10);
  write_i2c_reg(I2C_R3_CLOCK, 7, 1, (((result->hsdif - 4) & 0x7) << 5) | (((result->w -1) & 0x7F) >> 2));
  write_i2c_reg(I2C_R3_CLOCK, 8, 1, ((((result->w)-1) & 0x3) << 6) | ( (unsigned int) (rfreq_hex >>32) & 0x3F));
  write_i2c_reg(I2C_R3_CLOCK, 9, 1, (rfreq_hex >> 24) & 0xFF);
  write_i2c_reg(I2C_R3_CLOCK, 10, 1, (rfreq_hex >> 16) & 0xFF);
  write_i2c_reg(I2C_R3_CLOCK, 11, 1, (rfreq_hex >> 8) & 0xFF);
  write_i2c_reg(I2C_R3_CLOCK, 12, 1, (rfreq_hex) & 0xFF);

  // Remember old settings
  old_r135 = read_i2c_reg(I2C_R3_CLOCK,135,1);
  new_r135 = (old_r135 & 0xFF) | 0x40;

  // Unfreeze DCO
  write_i2c_reg(I2C_R3_CLOCK, 137, 1, 0X00);
  write_i2c_reg(I2C_R3_CLOCK, 135, 1, new_r135);

}

void htif_zedboard_t::read_voltage(short supply_name)
{
  read_i2c_reg(supply_name,0,1);
}

float htif_zedboard_t::read_sense_voltage(short supply_name)
{
  uintptr_t rdata;
  float measured_voltage;

  // Set X8 gain to measure up to 1A
  write_i2c_reg(supply_name, 0x0A, 1, 3);
  rdata = read_i2c_reg(supply_name, 0x02, 2);
  measured_voltage = ((unsigned short) rdata >> 4)*57.3/4096.0;
  printf("voltage: %f\n",measured_voltage);
  return measured_voltage;
}

float htif_zedboard_t::read_sense_current(short supply_name)
{
  uintptr_t rdata;
  float measured_current;

  // Set ADC mux
  write_i2c_reg(supply_name, 0x0A, 1, 2);
  rdata = read_i2c_reg(supply_name, 0x02, 2);
  measured_current = (((unsigned short) rdata >> 4)*13.44e-6)/0.05;
  printf("current: %f\n",measured_current);
  return measured_current;
}


void htif_zedboard_t::set_reference_voltage(short reference_name, float vdd_value)
{
  short dac_code;

  write_i2c_reg(I2C_R3_DAC, 0x72, 2, 0);
  printf("scale: %f\n",((float) (1 << 12))/2.048);
  printf("vdd_value: %f\n",vdd_value);
  dac_code = (short) (vdd_value*((float) (1 << 12))/2.048);
  printf("dac_code: %d\n",dac_code);
  write_i2c_reg(I2C_R3_DAC,0x30 | (reference_name & 0xF),2,dac_code << 4);

}

void htif_zedboard_t::set_voltage(short supply_name, float vdd_value)
{
  float r2;
  unsigned short wdata;

  if((supply_name == I2C_R3_VDDLO) | (supply_name == I2C_R3_VDD10))
  {
    if (vdd_value - 1.2 > 0)
    {
      printf("Tried to set too high a Vdd...\n");
      vdd_value = 1.0;
    }
  }

  r2 = (vdd_value/0.2-1)*1240;
  wdata = (unsigned short) (r2-40)*256/10000;
  write_i2c_reg(supply_name, 0, 1, wdata);
}

ssize_t htif_zedboard_t::write(const void* buf, size_t size)
{
  const uint32_t* x = (const uint32_t*)buf;
//  printf("write(%d) : ", size);
//  for (int i=0;i<size/4;i++)
//    printf("%08x ", x[i]);
//  printf("\n");
  assert(size >= sizeof(*x));

  for (unsigned int i=0;i<size/4;i++)
    write_reg(0, x[i]);

  return size;
}

ssize_t htif_zedboard_t::read(void* buf, size_t max_size)
{
  uint32_t* x = (uint32_t*)buf;
  assert(max_size >= sizeof(*x));

  // fifo data counter
  uintptr_t c = read_reg(1); 
  uint32_t count = 0;
  if (c > 0)
    for (count=0; count<c && count*sizeof(*x)<max_size; count++)
      x[count] = read_reg(0);

  return count*sizeof(*x);
}
