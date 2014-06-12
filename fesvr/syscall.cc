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
  uint32_t mode;
  uint32_t nlink;
  uint32_t uid;
  uint32_t gid;
  uint64_t rdev;
  uint64_t __pad1;
  uint64_t size;
  uint32_t blksize;
  uint32_t __pad2;
  uint64_t blocks;
  uint64_t atime;
  uint64_t __pad3;
  uint64_t mtime;
  uint64_t __pad4;
  uint64_t ctime;
  uint64_t __pad5;
  uint32_t __unused4;
  uint32_t __unused5;

  riscv_stat(const struct stat& s)
    : dev(s.st_dev), ino(s.st_ino), mode(s.st_mode), nlink(s.st_nlink),
      uid(s.st_uid), gid(s.st_gid), rdev(s.st_rdev), __pad1(0),
      size(s.st_size), blksize(s.st_blksize), __pad2(0),
      blocks(s.st_blocks), atime(s.st_atime), __pad3(0),
      mtime(s.st_mtime), __pad4(0), ctime(s.st_ctime), __pad5(0),
      __unused4(0), __unused5(0) {}
};

syscall_t::syscall_t(htif_t* htif)
  : htif(htif), memif(&htif->memif()), table(2048)
{
  table[93] = &syscall_t::sys_exit;
  table[63] = &syscall_t::sys_read;
  table[64] = &syscall_t::sys_write;
  table[1024] = &syscall_t::sys_open;
  table[56] = &syscall_t::sys_openat;
  table[57] = &syscall_t::sys_close;
  table[80] = &syscall_t::sys_fstat;
  table[62] = &syscall_t::sys_lseek;
  table[1038] = &syscall_t::sys_stat;
  table[1039] = &syscall_t::sys_lstat;
  table[79] = &syscall_t::sys_fstatat;
  table[1033] = &syscall_t::sys_access;
  table[48] = &syscall_t::sys_faccessat;
  table[25] = &syscall_t::sys_fcntl;
  table[1025] = &syscall_t::sys_link;
  table[1026] = &syscall_t::sys_unlink;
  table[1030] = &syscall_t::sys_mkdir;
  table[17] = &syscall_t::sys_getcwd;
  table[67] = &syscall_t::sys_pread;
  table[68] = &syscall_t::sys_pwrite;
  table[2011] = &syscall_t::sys_getmainvars;

  register_command(0, std::bind(&syscall_t::handle_syscall, this, _1), "syscall");

  int stdin_fd = dup(0), stdout_fd0 = dup(1), stdout_fd1 = dup(1);
  if (stdin_fd < 0 || stdout_fd0 < 0 || stdout_fd1 < 0)
    throw std::runtime_error("could not dup stdin/stdout");

  fds.alloc(stdin_fd); // stdin -> stdin
  fds.alloc(stdout_fd0); // stdout -> stdout
  fds.alloc(stdout_fd1); // stderr -> stdout
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

reg_t syscall_t::sys_exit(reg_t code, reg_t a1, reg_t a2, reg_t a3, reg_t a4)
{
  htif->exitcode = code << 1 | 1;
  return 0;
}

static reg_t sysret_errno(sreg_t ret)
{
  return ret == -1 ? -errno : ret;
}

reg_t syscall_t::sys_open(reg_t pname, reg_t len, reg_t flags, reg_t mode, reg_t a4)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  int fd = open(&name[0], flags, mode);
  if (fd < 0)
    return sysret_errno(-1);
  return fds.alloc(fd);
}

reg_t syscall_t::sys_openat(reg_t dirfd, reg_t pname, reg_t len, reg_t flags, reg_t mode)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  int fd = openat(fds.lookup(dirfd), &name[0], flags, mode);
  if (fd < 0)
    return sysret_errno(-1);
  return fds.alloc(fd);
}

reg_t syscall_t::sys_read(reg_t fd, reg_t pbuf, reg_t len, reg_t a3, reg_t a4)
{
  std::vector<char> buf(len);
  ssize_t ret = read(fds.lookup(fd), &buf[0], len);
  reg_t ret_errno = sysret_errno(ret);
  if (ret > 0)
    memif->write(pbuf, ret, &buf[0]);
  return ret_errno;
}

reg_t syscall_t::sys_pread(reg_t fd, reg_t pbuf, reg_t len, reg_t off, reg_t a4)
{
  std::vector<char> buf(len);
  ssize_t ret = pread(fds.lookup(fd), &buf[0], len, off);
  reg_t ret_errno = sysret_errno(ret);
  if (ret > 0)
    memif->write(pbuf, ret, &buf[0]);
  return ret_errno;
}

reg_t syscall_t::sys_write(reg_t fd, reg_t pbuf, reg_t len, reg_t a3, reg_t a4)
{
  std::vector<char> buf(len);
  memif->read(pbuf, len, &buf[0]);
  reg_t ret = sysret_errno(write(fds.lookup(fd), &buf[0], len));
  return ret;
}

reg_t syscall_t::sys_pwrite(reg_t fd, reg_t pbuf, reg_t len, reg_t off, reg_t a4)
{
  std::vector<char> buf(len);
  memif->read(pbuf, len, &buf[0]);
  reg_t ret = sysret_errno(pwrite(fds.lookup(fd), &buf[0], len, off));
  return ret;
}

reg_t syscall_t::sys_close(reg_t fd, reg_t a1, reg_t a2, reg_t a3, reg_t a4)
{
  if (close(fds.lookup(fd)) < 0)
    return sysret_errno(-1);
  fds.dealloc(fd);
  return 0;
}

reg_t syscall_t::sys_lseek(reg_t fd, reg_t ptr, reg_t dir, reg_t a3, reg_t a4)
{
  return sysret_errno(lseek(fds.lookup(fd), ptr, dir));
}

reg_t syscall_t::sys_fstat(reg_t fd, reg_t pbuf, reg_t a2, reg_t a3, reg_t a4)
{
  struct stat buf;
  reg_t ret = sysret_errno(fstat(fds.lookup(fd), &buf));
  if (ret != (reg_t)-1)
  {
    riscv_stat rbuf(buf);
    memif->write(pbuf, sizeof(rbuf), &rbuf);
  }
  return ret;
}

reg_t syscall_t::sys_fcntl(reg_t fd, reg_t cmd, reg_t arg, reg_t a3, reg_t a4)
{
  return sysret_errno(fcntl(fds.lookup(fd), cmd, arg));
}

reg_t syscall_t::sys_stat(reg_t pname, reg_t len, reg_t pbuf, reg_t a3, reg_t a4)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);

  struct stat buf;
  reg_t ret = sysret_errno(stat(&name[0], &buf));
  if (ret != (reg_t)-1)
  {
    riscv_stat rbuf(buf);
    memif->write(pbuf, sizeof(rbuf), &rbuf);
  }
  return ret;
}

reg_t syscall_t::sys_lstat(reg_t pname, reg_t len, reg_t pbuf, reg_t a3, reg_t a4)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);

  struct stat buf;
  reg_t ret = sysret_errno(lstat(&name[0], &buf));
  riscv_stat rbuf(buf);
  if (ret != (reg_t)-1)
  {
    riscv_stat rbuf(buf);
    memif->write(pbuf, sizeof(rbuf), &rbuf);
  }
  return ret;
}

reg_t syscall_t::sys_fstatat(reg_t dirfd, reg_t pname, reg_t len, reg_t pbuf, reg_t flags)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);

  struct stat buf;
  reg_t ret = sysret_errno(fstatat(fds.lookup(dirfd), &name[0], &buf, flags));
  if (ret != (reg_t)-1)
  {
    riscv_stat rbuf(buf);
    memif->write(pbuf, sizeof(rbuf), &rbuf);
  }
  return ret;
}

reg_t syscall_t::sys_access(reg_t pname, reg_t len, reg_t mode, reg_t a3, reg_t a4)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  return sysret_errno(access(&name[0], mode));
}

reg_t syscall_t::sys_faccessat(reg_t dirfd, reg_t pname, reg_t len, reg_t mode, reg_t flags)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  return sysret_errno(faccessat(fds.lookup(dirfd), &name[0], mode, flags));
}

reg_t syscall_t::sys_link(reg_t poname, reg_t olen, reg_t pnname, reg_t nlen, reg_t a4)
{
  std::vector<char> oname(olen), nname(nlen);
  memif->read(poname, olen, &oname[0]);
  memif->read(pnname, nlen, &nname[0]);
  return sysret_errno(link(&oname[0], &nname[0]));
}

reg_t syscall_t::sys_unlink(reg_t pname, reg_t len, reg_t a2, reg_t a3, reg_t a4)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  return sysret_errno(unlink(&name[0]));
}

reg_t syscall_t::sys_mkdir(reg_t pname, reg_t len, reg_t mode, reg_t a3, reg_t a4)
{
  std::vector<char> name(len);
  memif->read(pname, len, &name[0]);
  return sysret_errno(mkdir(&name[0], mode));
}

reg_t syscall_t::sys_getcwd(reg_t pbuf, reg_t size, reg_t a2, reg_t a3, reg_t a4)
{
  std::vector<char> buf(size);
  char* ret = getcwd(&buf[0], size);
  if(ret) {
    memif->write(pbuf, size, &buf[0]);
  }
  return pbuf;
}

reg_t syscall_t::sys_getmainvars(reg_t pbuf, reg_t limit, reg_t a2, reg_t a3, reg_t a4)
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
    return -ENOMEM;

  memif->write(pbuf, bytes.size(), &bytes[0]);
  return 0;
}

void syscall_t::dispatch(reg_t mm)
{
  reg_t magicmem[8];
  memif->read(mm, sizeof(magicmem), magicmem);

  reg_t n = magicmem[0];
  if (n >= table.size() || !table[n])
    throw std::runtime_error("bad syscall #" + std::to_string(n));

  magicmem[0] = (this->*table[n])(magicmem[1], magicmem[2], magicmem[3], magicmem[4], magicmem[5]);

  memif->write(mm, sizeof(magicmem), magicmem);
}

reg_t fds_t::alloc(int fd)
{
  reg_t i;
  for (i = 0; i < fds.size(); i++)
    if (fds[i] == -1)
      break;

  if (i == fds.size())
    fds.resize(i+1);

  fds[i] = fd;
  return i;
}

void fds_t::dealloc(reg_t fd)
{
  fds[fd] = -1;
}

int fds_t::lookup(reg_t fd)
{
  return fd >= fds.size() ? -1 : fds[fd];
}
