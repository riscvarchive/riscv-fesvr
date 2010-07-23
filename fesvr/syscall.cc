#include "memif.h"
#include "syscall.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

struct sysret_t
{
  reg_t result;
  reg_t err;
};

sysret_t appsys_exit(htif_t* htif, memif_t* memif, reg_t code)
{
  htif->stop();
  exit(code);
}

sysret_t appsys_open(htif_t* htif, memif_t* memif,
                  addr_t pname, reg_t len, reg_t flags, reg_t mode)
{
  char name[len];
  memif->read(pname,len,(uint8_t*)name);
  return (sysret_t){open(name,mode,flags,mode),errno};
}

sysret_t appsys_read(htif_t* htif, memif_t* memif,
                     reg_t fd, addr_t pbuf, reg_t len)
{
  char* buf = (char*)malloc(len);
  demand(buf,"malloc failure!");
  sysret_t ret = {read(fd,buf,len),errno};
  if(ret.result != (reg_t)-1)
    memif->write(pbuf,ret.result,(uint8_t*)buf);
  free(buf);
  return ret;
}

sysret_t appsys_write(htif_t* htif, memif_t* memif,
                      reg_t fd, addr_t pbuf, reg_t len)
{
  char* buf = (char*)malloc(len);
  demand(buf,"malloc failure!");
  memif->read(pbuf,len,(uint8_t*)buf);
  sysret_t ret = {write(fd,buf,len),errno};
  free(buf);
  return ret;
}

sysret_t appsys_close(htif_t* htif, memif_t* memif,
                      reg_t fd)
{
  return (sysret_t){close(fd),errno};
}

sysret_t appsys_lseek(htif_t* htif, memif_t* memif,
                      reg_t fd, reg_t ptr, reg_t dir)
{
  return (sysret_t){lseek(fd,ptr,dir),errno};
}

sysret_t appsys_fstat(htif_t* htif, memif_t* memif,
                     reg_t fd, addr_t pbuf)
{
  struct stat buf;
  sysret_t ret = {fstat(fd,&buf),errno};
  if(ret.result != (reg_t)-1)
    memif->write(pbuf,sizeof(buf),(uint8_t*)&buf);
  return ret;
}

sysret_t appsys_stat(htif_t* htif, memif_t* memif,
                     addr_t pname, reg_t len, reg_t fd, addr_t pbuf)
{
  char name[len];
  memif->read(pname,len,(uint8_t*)name);

  struct stat buf;
  sysret_t ret = {stat(name,&buf),errno};
  if(ret.result != (reg_t)-1)
    memif->write(pbuf,sizeof(buf),(uint8_t*)&buf);
  return ret;
}

sysret_t appsys_lstat(htif_t* htif, memif_t* memif,
                     addr_t pname, reg_t len, reg_t fd, addr_t pbuf)
{
  char name[len];
  memif->read(pname,len,(uint8_t*)name);

  struct stat buf;
  sysret_t ret = {lstat(name,&buf),errno};
  if(ret.result != (reg_t)-1)
    memif->write(pbuf,sizeof(buf),(uint8_t*)&buf);
  return ret;
}

sysret_t appsys_link(htif_t* htif, memif_t* memif,
                  addr_t poname, reg_t olen, addr_t pnname, reg_t nlen)
{
  char oname[olen],nname[nlen];
  memif->read(poname,olen,(uint8_t*)oname);
  memif->read(pnname,nlen,(uint8_t*)nname);
  return (sysret_t){link(oname,nname),errno};
}

sysret_t appsys_unlink(htif_t* htif, memif_t* memif,
                  addr_t pname, reg_t len)
{
  char name[len];
  memif->read(pname,len,(uint8_t*)name);
  return (sysret_t){unlink(name),errno};
}

typedef sysret_t (*syscall_t)(htif_t*,memif_t*,reg_t,reg_t,reg_t,reg_t);

void dispatch_syscall(htif_t* htif, memif_t* memif, addr_t mm)
{
  reg_t magicmem[5];
  memif->read(mm,sizeof(magicmem),(uint8_t*)magicmem);

  void* syscall_table[256];
  memset(syscall_table, 0, sizeof(syscall_table));
  syscall_table[1] = (void*)appsys_exit;
  syscall_table[3] = (void*)appsys_read;
  syscall_table[4] = (void*)appsys_write;
  syscall_table[5] = (void*)appsys_open;
  syscall_table[6] = (void*)appsys_close;
  syscall_table[28] = (void*)appsys_fstat;
  syscall_table[19] = (void*)appsys_lseek;
  syscall_table[18] = (void*)appsys_stat;
  syscall_table[84] = (void*)appsys_lstat;
  syscall_table[9] = (void*)appsys_link;
  syscall_table[10] = (void*)appsys_unlink;

  syscall_t p;
  reg_t n = magicmem[0];
  if(n >= sizeof(syscall_table)/sizeof(void*) || !syscall_table[n])
    demand(0,"bad syscall #%ld!",n);
  else
    p = (syscall_t)syscall_table[n];

  sysret_t ret = p(htif,memif,magicmem[1],magicmem[2],magicmem[3],magicmem[4]);

  memif->write(mm,sizeof(ret),(uint8_t*)&ret);
}
