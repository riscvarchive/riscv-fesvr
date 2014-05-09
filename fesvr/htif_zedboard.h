#ifndef __HTIF_ZEDBOARD_H
#define __HTIF_ZEDBOARD_H

#include "htif.h"
#include <vector>
#include "clk_lookup.h"

#define I2C_DIVISOR 3
#define I2C_WDATA 4
#define I2C_SLAVE_ADDR 5
#define I2C_REG_ADDR 6
#define I2C_TOGGLE 7
#define I2C_RDATA 8
#define CLK_SEL_ADDR 9

#define I2C_R3_VDDHI 0x50
#define I2C_R3_VDDHI_MEAS 0x70
#define I2C_R3_VDDLO 0x52
#define I2C_R3_VDDLO_MEAS 0x78
#define I2C_R3_VDD18 0x51
#define I2C_R3_VDD18_MEAS 0x72
#define I2C_R3_VDD10 0x53
#define I2C_R3_VDD10_MEAS 0x7A
#define I2C_R3_DAC 0x1F
#define I2C_R3_CLOCK 0x55
#define I2C_R3_VDDS 3
#define I2C_R3_GNDS 2
#define I2C_R3_VREF 1
#define I2C_R3_VDD_REPLICA 0

int freq_compare(const void *c1, const void *c2);

class htif_zedboard_t : public htif_t
{
 public:
  htif_zedboard_t(const std::vector<std::string>& args);
  ~htif_zedboard_t();
  float get_host_clk_freq();
  void set_voltage(short supply_name, float vdd_value);
  void set_reference_voltage(short reference_name, float vdd_value);
  void write_i2c_reg(short supply_name, short reg_addr, short num_bytes, short wdata);
  short read_i2c_reg(short supply_name, short reg_addr, short num_bytes);
  void read_voltage(short supply_name);
  void write_clock(float freq);
  float read_sense_voltage(short supply_name);
  float read_sense_current(short supply_name);
  void set_i2c_divider(short divider);
  void set_clksel(int sel);
  void reset_internal();
  void st_sram_init();
  void bz_sram_init();
  void dcdc_init(unsigned int dcdc_conf);
  void cassia_init(unsigned int s1, unsigned int s2);
  void clock_init(int core_clksel, int cassia_clksel, int bist_clksel);
  void bz_sram_init(unsigned int saen_width_ctrl, unsigned int write_delay_ctrl, unsigned int write_timing_sel, unsigned int saen_sel, unsigned int use_sa,unsigned int use_fbb,unsigned int n_vref_ctrl,unsigned int saen_delay_ctrl,unsigned int bl_boost_ctrl);

 protected:
  ssize_t read(void* buf, size_t max_size);
  ssize_t write(const void* buf, size_t size);

  size_t chunk_max_size() { return 64; }
  size_t chunk_align() { return 64; }
  uint32_t mem_mb() { return 256; }
  uint32_t num_cores() { return 1; } // FIXME

 private:
  volatile uintptr_t* dev_vaddr;
  const static uintptr_t dev_paddr = 0x43C00000;
};

#endif // __HTIF_ZEDBOARD_H
