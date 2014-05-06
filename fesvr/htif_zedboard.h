#ifndef __HTIF_ZEDBOARD_H
#define __HTIF_ZEDBOARD_H

#include "htif.h"
#include <vector>

#define I2C_DIVISOR 3
#define I2C_WDATA 4
#define I2C_SLAVE_ADDR 5
#define I2C_REG_ADDR 6
#define I2C_TOGGLE 7
#define I2C_RDATA 8

#define I2C_R3_VDDHI 0x20
#define I2C_R3_VDDHI_MEAS 0x70
#define I2C_R3_VDDLO 0x22
#define I2C_R3_VDDLO_MEAS 0x78
#define I2C_R3_VDD18 0x21
#define I2C_R3_VDD18_MEAS 0x72
#define I2C_R3_VDD10 0x23
#define I2C_R3_VDD10_MEAS 0x7A
#define I2C_R3_DAC 0x1F

class htif_zedboard_t : public htif_t
{
 public:
  htif_zedboard_t(const std::vector<std::string>& args);
  ~htif_zedboard_t();
  float get_host_clk_freq();
  void set_voltage(short supply_name, float vdd_value);
  void read_voltage(short supply_name);
  void set_i2c_divider(short divider);
  void reset_internal();

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
