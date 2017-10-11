#include "dtm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
//F1 includes to deal with the fpga
#include "fpga_pci.h"
#include "fpga_mgmt.h"

class f1_dtm_t : public dtm_t
{
  public:
   f1_dtm_t(const std::vector<std::string>& args);
   ~f1_dtm_t();

  uint32_t read(uint32_t addr) override;
  uint32_t write(uint32_t addr, uint32_t data) override;
  void nop() override;
}


/*
 * pci_vendor_id and pci_device_id values below are Amazon's and avaliable to use for a given FPGA slot. 
 * Users may replace these with their own if allocated to them by PCI SIG
 */
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */

f1_dtm_t::f1_dtm_t(const std::vector<std::string>& args) : dtm_t(args)
{
  int ret_code;
  int slot_id = 0; //TODO: make this fesvr arg
  /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
  ret_code = fpga_pci_init();
  if(ret_code) throw std::runtime_error("FESVR F1_DTM: FPGA PCI init failed!!");
  /* check that AFI is loaded */
  struct fpga_mgmt_image_info info = {0};
  ret_code = fpga_mgmt_describe_local_image(slot_id, &info, 0);
  if(ret_code) throw std::runtime_error("FESVR F1_DTM: Unable to get FPGA AFI info! Are you running as root?");
  if(info.status != FPGA_STATUS_LOADED) throw std::runtime_erro("FESVR F1_DTM: FPGA is not ready!")r
  /* confirm that the AFI that we expect is in fact loaded */
  if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
      info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
      /* AFI does not show expected PCI vendor id and device ID. If the AFI
       * was just loaded, it might need a rescan. Rescanning now.
       */
      ret_code = fpga_pci_rescan_slot_app_pfs(slot_id);
      if(ret_code) throw std::runtime_error("FESVR F1_DTM: Unable to update FPGA PF!");
      /* get local image description, contains status, vendor id, and device id. */
      ret_code = fpga_mgmt_describe_local_image(slot_id, &info, 0);
      if(ret_code) throw std::runtime_error("FESVR F1_DTM: Unable to get FPGA AFI information!");
      /* confirm that the AFI that we expect is in fact loaded after rescan */
      if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
           info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
          throw std::runtime_error("FESVR F1_DTM: The PCI vendor id and device of the loaded FPGA AFI are not the expected values.");
      }
  }

}

uint32_t f1_dtm_t::read(uint32_t addr) {
}

uint32_t f1_dtm_t::write(uint32_t addr, uint32_t data) {
}

uint32_t f1_dtm_t::nop() {
  //TODO: is there anything to do here?
}
