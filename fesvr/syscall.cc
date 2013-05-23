// See LICENSE for license details.

#include "syscall.h"
#include "htif.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <termios.h>
#include <sstream>
#include <iostream>
using namespace std::placeholders;

struct riscv_stat
{
  uint64_t dev;
  uint64_t ino;
  uint64_t nlink;
  uint64_t mode;
  uint64_t uid;
  uint64_t gid;
  uint64_t rdev;
  uint64_t size;
  uint64_t blksize;
  uint64_t blocks;
  uint64_t atime;
  uint64_t mtime;
  uint64_t ctime;

  riscv_stat(const struct stat& s)
    : dev(s.st_dev), ino(s.st_ino), nlink(s.st_nlink), mode(s.st_mode),
      uid(s.st_uid), gid(s.st_gid), rdev(s.st_rdev), size(s.st_size),
      blksize(s.st_blksize), blocks(s.st_blocks), atime(s.st_atime),
      mtime(s.st_mtime), ctime(s.st_ctime) {}
};

syscall_t::syscall_t(htif_t* htif)
  : htif(htif), memif(&htif->memif()), table(256)
{
  table[1] = &syscall_t::sys_exit;
  table[3] = &syscall_t::sys_read;
  table[4] = &syscall_t::sys_write;
  table[5] = &syscall_t::sys_open;
  table[6] = &syscall_t::sys_close;
  table[28] = &syscall_t::sys_fstat;
  table[19] = &syscall_t::sys_lseek;
  table[18] = &syscall_t::sys_stat;
  table[84] = &syscall_t::sys_lstat;
  table[9] = &syscall_t::sys_link;
  table[10] = &syscall_t::sys_unlink;
  table[180] = &syscall_t::sys_pread;
  table[181] = &syscall_t::sys_pwrite;
  table[201] = &syscall_t::sys_getmainvars;

  register_command(0, std::bind(&syscall_t::handle_syscall, this, _1), "syscall");
}

void syscall_t::handle_syscall(command_t cmd)
{
  if (cmd.payload() & 1) // test pass/fail
  {
    htif->exitcode = cmd.payload();
    if (htif->exit_code())
      std::cerr << "*** FAILED *** (tohost = " << htif->exit_code() << ")" << std::endl;
  }
  else // proxied system call
    dispatch(cmd.payload());

  cmd.respond(1);
}

sysret_t syscall_t::sys_exit(reg_t code, reg_t a1, reg_t a2, reg_t a3)
{
  htif->exitcode = code << 1 | 1;
  return (sysret_t){0, 0};
}

sysret_t syscall_t::sys_open(reg_t pname, reg_t len, reg_t flags, reg_t mode)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  return (sysret_t){open(&name[0], flags, mode), errno};
}

sysret_t syscall_t::sys_read(reg_t fd, reg_t pbuf, reg_t len, reg_t a3)
{
  std::vector<char> buf(len);
  sysret_t ret = {read(fd, &buf[0], len), errno};
  if(ret.result != -1)
    memif->write(pbuf, ret.result, &buf[0]);
  return ret;
}

sysret_t syscall_t::sys_pread(reg_t fd, reg_t pbuf, reg_t len, reg_t off)
{
  std::vector<char> buf(len);
  sysret_t ret = {pread(fd, &buf[0], len, off), errno};
  if(ret.result != -1)
    memif->write(pbuf, ret.result, &buf[0]);
  return ret;
}

sysret_t syscall_t::sys_write(reg_t fd, reg_t pbuf, reg_t len, reg_t a3)
{
  std::vector<char> buf(len);
  memif->read(pbuf, len, &buf[0]);
  sysret_t ret = {write(fd, &buf[0], len), errno};
  return ret;
}

sysret_t syscall_t::sys_pwrite(reg_t fd, reg_t pbuf, reg_t len, reg_t off)
{
  std::vector<char> buf(len);
  memif->read(pbuf, len, &buf[0]);
  sysret_t ret = {pwrite(fd, &buf[0], len, off), errno};
  return ret;
}

sysret_t syscall_t::sys_close(reg_t fd, reg_t a1, reg_t a2, reg_t a3)
{
  return (sysret_t){close(fd), errno};
}

sysret_t syscall_t::sys_lseek(reg_t fd, reg_t ptr, reg_t dir, reg_t a3)
{
  return (sysret_t){lseek(fd, ptr, dir), errno};
}

sysret_t syscall_t::sys_fstat(reg_t fd, reg_t pbuf, reg_t a2, reg_t a3)
{
  struct stat buf;
  sysret_t ret = {fstat(fd, &buf), errno};
  if (ret.result != -1)
  {
    riscv_stat rbuf(buf);
    memif->write(pbuf, sizeof(rbuf), &rbuf);
  }
  return ret;
}

sysret_t syscall_t::sys_stat(reg_t pname, reg_t len, reg_t pbuf, reg_t a3)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);

  struct stat buf;
  sysret_t ret = {stat(&name[0], &buf), errno};
  if (ret.result != -1)
  {
    riscv_stat rbuf(buf);
    memif->write(pbuf, sizeof(rbuf), &rbuf);
  }
  return ret;
}

sysret_t syscall_t::sys_lstat(reg_t pname, reg_t len, reg_t pbuf, reg_t a3)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);

  struct stat buf;
  sysret_t ret = {lstat(&name[0], &buf), errno};
  riscv_stat rbuf(buf);
  if (ret.result != -1)
  {
    riscv_stat rbuf(buf);
    memif->write(pbuf, sizeof(rbuf), &rbuf);
  }
  return ret;
}

sysret_t syscall_t::sys_link(reg_t poname, reg_t olen, reg_t pnname, reg_t nlen)
{
  std::vector<char> oname(olen), nname(nlen);
  memif->read(poname, olen, &oname[0]);
  memif->read(pnname, nlen, &nname[0]);
  return (sysret_t){link(&oname[0], &nname[0]), errno};
}

sysret_t syscall_t::sys_unlink(reg_t pname, reg_t len, reg_t a2, reg_t a3)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  return (sysret_t){unlink(&name[0]), errno};
}

sysret_t syscall_t::sys_getmainvars(reg_t pbuf, reg_t limit, reg_t a2, reg_t a3)
{
  std::vector<std::string> args = htif->target_args();
  std::vector<uint64_t> words(args.size() + 3);
  words[0] = args.size();
  words[args.size()+1] = 0; // argv[argc] = NULL
  words[args.size()+2] = 0; // envp[0] = NULL

  size_t sz = (args.size() + 3) * sizeof(words[0]);
  for (size_t i = 0; i < args.size(); i++)
  {
    words[i+1] = sz + pbuf;
    sz += args[i].length() + 1;
  }

  std::vector<char> bytes(sz);
  memcpy(&bytes[0], &words[0], sizeof(words[0]) * words.size());
  for (size_t i = 0; i < args.size(); i++)
    strcpy(&bytes[words[i+1] - pbuf], args[i].c_str());

  if (bytes.size() > limit)
    return (sysret_t){-1, ENOMEM};

  memif->write(pbuf, bytes.size(), &bytes[0]);
  return (sysret_t){0, 0};
}

void syscall_t::dispatch(reg_t mm)
{
  reg_t magicmem[8];
  memif->read(mm, sizeof(magicmem), magicmem);

  reg_t n = magicmem[0];
  if (n >= table.size() || !table[n])
    throw std::runtime_error("bad syscall #" + std::to_string(n));

  sysret_t ret = (this->*table[n])(magicmem[1], magicmem[2], magicmem[3], magicmem[4]);

  magicmem[0] = ret.result;
  magicmem[1] = ret.err;
  memif->write(mm, sizeof(magicmem), magicmem);
}
