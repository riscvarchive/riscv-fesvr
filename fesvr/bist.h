#ifndef _BIST_EMULATOR_H
#define _BIST_EMULATOR_H

#include <fstream>
#include <map>

#define LAST(k,n) ((k) & ((1<<(n))-1))
#define MID(k,m,n) LAST((k)>>(m),((n)-(m)))

struct HTIF_split
{
  size_t waddr;
  reg_t startpos;
  reg_t endpos;
};


class bist_t
{
   public:
    bist_t(htif_zedboard_t* htif,const std::vector<std::string>& args)
      : htif(htif) 
    {
      set_bist_fname(args);
    }

     void run_bist(){

      std::map<std::string, size_t> cr_mapping;
      std::map<std::string, std::vector<HTIF_split>> split_cr_mapping;
      std::vector<HTIF_split> container;
      std::vector<HTIF_split>::iterator p;


      std::string line;
      std::string wdata_string;
      std::string longhex;
      size_t pos;
      size_t hex_pos;
      size_t waddr;
      reg_t wdata;
      reg_t pollresponse;
      uint64_t upper_buffer;
      uint64_t lower_buffer;
      uint64_t zi;


      // Copied from python bist_gen_htif_hookups.py
      cr_mapping["bist_clksel"] = 56;
      cr_mapping["bist_reset"] = 111; 
      HTIF_split bist_test0_din_split_71_64 = {64, 71, 64};
      split_cr_mapping["bist_test0_din"].push_back(bist_test0_din_split_71_64);
      HTIF_split bist_test0_din_split_63_0 = {65, 63, 0};
      split_cr_mapping["bist_test0_din"].push_back(bist_test0_din_split_63_0);

      HTIF_split bist_test1_din_split_71_64 = {66, 71, 64};
      split_cr_mapping["bist_test1_din"].push_back(bist_test1_din_split_71_64);
      HTIF_split bist_test1_din_split_63_0 = {67, 63, 0};
      split_cr_mapping["bist_test1_din"].push_back(bist_test1_din_split_63_0);

      HTIF_split bist_test2_din_split_71_64 = {80, 71, 64};
      split_cr_mapping["bist_test2_din"].push_back(bist_test2_din_split_71_64);
      HTIF_split bist_test2_din_split_63_0 = {81, 63, 0};
      split_cr_mapping["bist_test2_din"].push_back(bist_test2_din_split_63_0);

      HTIF_split bist_test3_din_split_71_64 = {82, 71, 64};
      split_cr_mapping["bist_test3_din"].push_back(bist_test3_din_split_71_64);
      HTIF_split bist_test3_din_split_63_0 = {83, 63, 0};
      split_cr_mapping["bist_test3_din"].push_back(bist_test3_din_split_63_0);

      HTIF_split bist_test4_din_split_71_64 = {84, 71, 64};
      split_cr_mapping["bist_test4_din"].push_back(bist_test4_din_split_71_64);
      HTIF_split bist_test4_din_split_63_0 = {94, 63, 0};
      split_cr_mapping["bist_test4_din"].push_back(bist_test4_din_split_63_0);



      HTIF_split bist_safe_write_din_split_71_64 = {68, 71, 64};
      split_cr_mapping["bist_safe_write_din"].push_back(bist_safe_write_din_split_71_64);
      HTIF_split bist_safe_write_din_split_63_0 = {69, 63, 0};
      split_cr_mapping["bist_safe_write_din"].push_back(bist_safe_write_din_split_63_0);
      cr_mapping["bist_memory_select"] = 70;
      cr_mapping["bist_safe_write_start_address"] = 71;
      cr_mapping["bist_safe_write_end_address"] = 72;
      cr_mapping["bist_safe_write_step_bit"] = 73;
      cr_mapping["bist_safe_read_start_address"] = 74;
      cr_mapping["bist_safe_read_end_address"] = 75;
      cr_mapping["bist_safe_read_step_bit"] = 76;
      cr_mapping["bist_test0_start_address"] = 77;
      cr_mapping["bist_test0_end_address"] = 78;
      cr_mapping["bist_test0_step_bit"] = 79;
      cr_mapping["bist_alternate"] = 93;
      cr_mapping["bist_test0_compare_to"] = 85;
      cr_mapping["bist_test1_compare_to"] = 86;
      cr_mapping["bist_test2_compare_to"] = 90;
      cr_mapping["bist_test3_compare_to"] = 91;
      cr_mapping["bist_test4_compare_to"] = 92;
      cr_mapping["bist_safe_read_compare_to"] = 87;
      HTIF_split bist_atspeed_error_buffer_split_84_64 = {88, 84, 64};
      split_cr_mapping["bist_atspeed_error_buffer"].push_back(bist_atspeed_error_buffer_split_84_64);
      HTIF_split bist_atspeed_error_buffer_split_63_0 = {89, 63, 0};
      split_cr_mapping["bist_atspeed_error_buffer"].push_back(bist_atspeed_error_buffer_split_63_0);
      cr_mapping["bist_atspeed_error_buffer_sel"] = 95;
      cr_mapping["bist_atspeed_entry_count"] = 96;
      cr_mapping["bist_atspeed_exception_info"] = 97;
      cr_mapping["bist_safe_write_enable"] = 98;
      cr_mapping["bist_safe_write_voltage"] = 99;
      cr_mapping["bist_test0_enable"] = 100;
      cr_mapping["bist_test0_mode"] = 101;
      cr_mapping["bist_test1_enable"] = 102;
      cr_mapping["bist_test1_mode"] = 103;
      cr_mapping["bist_test2_enable"] = 112;
      cr_mapping["bist_test2_mode"] = 113;
      cr_mapping["bist_test3_enable"] = 114;
      cr_mapping["bist_test3_mode"] = 115;
      cr_mapping["bist_test4_enable"] = 116;
      cr_mapping["bist_test4_mode"] = 117;
      cr_mapping["bist_safe_read_enable"] = 104;
      cr_mapping["bist_safe_read_voltage"] = 105;
      cr_mapping["bist_begin_atspeed_test"] = 106;
      cr_mapping["bist_sram_vdd_is_safe"] = 107;
      cr_mapping["bist_entry_received"] = 108;
      cr_mapping["bist_transfer_output_scanchain"] = 109;
      cr_mapping["bist_want_safe_voltage"] = 110;
      cr_mapping["bzconf"] = 61;

      fprintf(stdout,"RUNNING BIST\n");
      fprintf(stdout,"Bist filename: %s\n",bistfname.c_str());
      bistfile.open(bistfname);
      while(bistfile.good()){
        getline(bistfile,line);
        pos = line.find("=");

        // See if write or poll
        // Can't poll on anything longer than 64 bits
        if (line.find("@") != std::string::npos){
          fprintf(stdout,"FOUND POLL\n");
          waddr = cr_mapping[line.substr(1,pos-1)];
          wdata_string = line.substr(pos+2);
          wdata = strtoul(wdata_string.c_str(), NULL, 10);
          fprintf(stdout,"Poll: %u == %llu\n",waddr,wdata);
          if (waddr==cr_mapping["bist_transfer_output_scanchain"]){
            printf("Setting safe voltage for scan\n");
            htif->set_voltage(I2C_R3_VDDLO,1.0);
            usleep(10000);

          }
          while(1){
            pollresponse = htif->read_cr(-1,waddr);
            if(wdata == pollresponse){
              break;
            }
          }
          // TODO: Move these outside to respond automatically?
          // Raise the voltage
          if(waddr==cr_mapping["bist_want_safe_voltage"] && wdata==1){
            printf("Setting safe voltage\n");
            htif->set_voltage(I2C_R3_VDDLO,1.0);
            usleep(10000);
            // Respond that safe
            htif->write_cr(-1,cr_mapping["bist_sram_vdd_is_safe"],1);
          }
          // Or lower it
          if((waddr==cr_mapping["bist_want_safe_voltage"]) && wdata==0){
            printf("Setting test voltage\n");
            htif->set_voltage(I2C_R3_VDDLO,vdd_backup);
            usleep(10000);
            // Respond that unsafe
            // TODO: only works with vdd_backup
            htif->write_cr(-1,cr_mapping["bist_sram_vdd_is_safe"],0);
          }


        } else if (line.find("*") != std::string::npos) {
          fprintf(stdout,"%s\n",line.c_str());
        } else if (line.find("#echo") != std::string::npos) {
          fprintf(stdout,"SIMPLE ECHO\n");
          htif->write_cr(-1,95,10);
          upper_buffer = htif->read_cr(-1,88);
          lower_buffer = htif->read_cr(-1,89);
          fprintf(stdout,"=\n%x,%x\n\n",upper_buffer,lower_buffer);

          fprintf(stdout,"FOUND ECHO\n");
          // For some weird reason, I couldn't do in pieces
          // any larger than 32 bits
          for(zi = 0; zi <= 31; zi++){
          htif->write_cr(-1,95,zi);
          upper_buffer = htif->read_cr(-1,bist_atspeed_error_buffer_split_84_64.waddr);
          lower_buffer = htif->read_cr(-1,bist_atspeed_error_buffer_split_63_0.waddr);
          fprintf(stdout,"%x,%x",upper_buffer,lower_buffer);
          fprintf(stdout,"Buffer: %llu, Addr: %u Dout: %06x%06x%06x, Test: %u\n",zi, (unsigned int) MID(upper_buffer,11,20), (unsigned int) ((MID(upper_buffer,0,10+1) << 13 ) | MID(lower_buffer,51,63+1)), (unsigned int)  MID(lower_buffer,27,50+1), (unsigned int) MID(lower_buffer,3,26+1), (unsigned int) MID(lower_buffer,0,2+1));
          }

        } else {
        wdata_string = line.substr(pos+1);
        hex_pos = wdata_string.find("x");
        // Anything greater than 64 bits must be written in HEX
        // Anything greater than 64 bits must have indexes % 4 = 0
        if (hex_pos != std::string::npos){
          longhex = wdata_string.substr(hex_pos+1);
          //if(longhex.size() < 64){
            wdata = strtoul(longhex.c_str(), NULL, 16);
          //} // if bigger, just parse longhex later
        } else {
          wdata = strtoul(wdata_string.c_str(), NULL, 10);
        }
        // Detect if normal or split entry
        std::map<std::string, std::vector<HTIF_split>>::iterator p = split_cr_mapping.find(line.substr(0,pos));
        std::vector<HTIF_split>::iterator p_splits;
        if (p != split_cr_mapping.end()){
          // Split entry
          // If found, p will point at a vector of HTIF_pairs,
          // Derefence and iterate over these
          container = split_cr_mapping[line.substr(0,pos)];
          for(p_splits = container.begin(); p_splits != container.end(); p_splits++){
            wdata = strtoul(longhex.substr(longhex.size() -1 - p_splits->startpos / 4, (p_splits->startpos / 4 - p_splits->endpos / 4) + 1).c_str(), NULL, 16);
            fprintf(stdout,"Line: %u %llu\n", p_splits->waddr, wdata);
            htif->write_cr(-1,p_splits->waddr,wdata);
          }
        } else {
          // Normal
          waddr = cr_mapping[line.substr(0,pos)];
          fprintf(stdout,"Line: %u, %llu\n",waddr,wdata);
          htif->write_cr(-1,waddr,wdata);
        }
       }

      }
     }

     void set_bist_fname(const std::vector<std::string>& args)
     {
      for (std::vector<std::string>::const_iterator a = args.begin(); a != args.end(); ++a)
      {
        if (a->substr(0, 5) == "+vdd=")
          vdd_backup = std::atof(a->substr(5).c_str());
        if (a->substr(0, 6) == "+bist=")
          bistfname = a->substr(6);
      }
     }

   private:
     htif_zedboard_t* htif;
     std::string bistfname;
     float vdd_backup;
     std::ifstream bistfile;


};

#endif
