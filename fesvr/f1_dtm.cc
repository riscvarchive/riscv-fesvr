#include "dtm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
//F1 includes to deal with the fpga
#include "fpga_pci.h"
#include "fpga_mgmt.h"

class f1_dtm_t : public dtm_t
{
  public:
   f1_dtm_t(const std::vector<std::string>& args);
   ~f1_dtm_t();

  uint32_t read(uint32_t addr) ;
  uint32_t write(uint32_t addr, uint32_t data) ;
  void nop() ;

  private:
    uint32_t do_command(dtm_t::req r);
    pci_bar_handle_t pci_bar_handle;
    req req_buf;
    resp resp_buf;
};


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
  if(info.status != FPGA_STATUS_LOADED) throw std::runtime_error("FESVR F1_DTM: FPGA is not ready!");
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
  /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */
  pci_bar_handle = PCI_BAR_HANDLE_INIT;
  ret_code = fpga_pci_attach(slot_id, FPGA_APP_PF, APP_PF_BAR4, 0, &pci_bar_handle);
  if(ret_code) throw std::runtime_error("FESVR F1_DTM: Unable to attach to the AFI");

}

uint32_t f1_dtm_t::do_command(dtm_t::req r)
{
  req_buf = r;
  //write request
  uint64_t request = ((uint64_t)(r.addr & 0x7f)<<34) | ((uint64_t)(r.data)<<2) | (r.op & 0x3);
  fpga_pci_poke64(pci_bar_handle, 0x1000, request);
  //read resp_register
  uint64_t response = 0;
  fpga_pci_peek64(pci_bar_handle, 0x1008, &response);
  resp_buf.resp = response & 0x3;
  resp_buf.data = (response>>2) & 0xffffffff;
  assert(resp_buf.resp == 0);
  return resp_buf.data;
}

uint32_t f1_dtm_t::read(uint32_t addr) {
  return do_command((req){addr, 1, 0});
}

uint32_t f1_dtm_t::write(uint32_t addr, uint32_t data) {
  return do_command((req){addr, 2, data});
}

void f1_dtm_t::nop() {
  do_command((req){0, 0, 0});
}
