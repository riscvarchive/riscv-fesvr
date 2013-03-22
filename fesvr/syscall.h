#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "packet.h"
#include <vector>
#include <string>

struct sysret_t
{
  sreg_t result;
  sreg_t err;
};

class syscall_t;
typedef sysret_t (syscall_t::*syscall_func_t)(reg_t, reg_t, reg_t, reg_t);

class htif_t;
class memif_t;

std::string itoa(int);

class syscall_t
{
 public:
  syscall_t(htif_t*);
  void dispatch(addr_t mm);
  
 private:
  htif_t* htif;
  memif_t* memif;

  std::vector<syscall_func_t> table;

  sysret_t sys_exit(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_open(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_read(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_pread(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_write(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_pwrite(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_close(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_lseek(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_fstat(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_stat(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_lstat(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_link(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_unlink(reg_t, reg_t, reg_t, reg_t);
  sysret_t sys_getmainvars(reg_t, reg_t, reg_t, reg_t);
};

#endif
