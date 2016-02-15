#include <stdexcept>

#include "blockdev.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void blockdev_t::open_file(const char *fname)
{
  off_t fsize;

  this->fd = open(fname, O_RDWR);
  if (this->fd < 0)
    throw std::runtime_error("could not open " + std::string(fname));

  fsize = lseek(this->fd, 0, SEEK_END);
  if (fsize == -1)
    throw std::runtime_error("could not determine size of file\n");
  if (fsize % SECTOR_SIZE != 0)
    throw std::runtime_error("block device not aligned to " + std::to_string(SECTOR_SIZE) + " bytes\n");
  lseek(this->fd, 0, SEEK_SET);
  this->max_sector = fsize / SECTOR_SIZE;
}

int blockdev_t::read_block(void *data)
{
  if (this->fd == -1)
    throw new std::runtime_error("tried to read from block device, no block device open\n");

  if (read(this->fd, data, SECTOR_SIZE) == -1)
    return -1;

  this->head += 1;

  return 0;
}

int blockdev_t::write_block(const void *data)
{
  if (this->fd == -1)
    throw new std::runtime_error("tried to write to block device, no block device open\n");

  if (write(this->fd, data, SECTOR_SIZE) == -1)
    return -1;

  this->head += 1;

  return 0;
}

int blockdev_t::set_head(uint64_t sector_num)
{
  uint64_t addr = sector_num * SECTOR_SIZE;

  if (sector_num > this->max_sector)
    return -1;

  if (lseek(this->fd, addr, SEEK_SET) == -1)
    return -1;

  return 0;
}

blockdev_t::blockdev_t(const std::vector<std::string>& args)
  : fd(-1), max_sector(0), head(0), _stop(false)
{
  std::vector<std::string> hargs;
  std::vector<std::string> targs;
  uint64_t i;

  for (i = 0; i < args.size(); i++)
    if (args[i].length() && args[i][0] != '-' && args[i][0] != '+')
      break;

  hargs.insert(hargs.begin(), args.begin(), args.begin() + i);
  targs.insert(targs.begin(), args.begin() + i, args.end());

  for (auto& arg : hargs) {
    if (arg.find("+blockdev=") == 0)
      this->open_file(arg.c_str() + 10);
  }
}

blockdev_t::~blockdev_t(void)
{
  close(this->fd);
}


void blockdev_t::process_read(uint64_t nsectors)
{
  uint64_t i, j;
  uint64_t data[SECTOR_SIZE / 8];
  uint64_t secnum;

  secnum = poll();

  if (set_head(secnum))
    throw std::runtime_error("invalid sector " + std::to_string(secnum));

  for (i = 0; i < nsectors; i++) {
    read_block(data);
    for (j = 0; j < SECTOR_SIZE / 8; j++)
      push(data[j]);
  }
}

void blockdev_t::process_write(uint64_t nsectors)
{
  uint64_t i, j;
  uint64_t data[SECTOR_SIZE / 8];
  uint64_t secnum;

  secnum = poll();

  if (set_head(secnum))
    throw std::runtime_error("invalid sector " + std::to_string(secnum));

  for (i = 0; i < nsectors; i++) {
    for (j = 0; j < SECTOR_SIZE / 8; j++)
      data[j] = poll();
    write_block(data);
  }
}

int blockdev_t::run(void)
{
  while (!this->_stop) {
    uint64_t in = poll();
    uint8_t cmd = in & 0xf;
    uint64_t nsectors = in >> 4;
    switch (cmd) {
    case BLOCKDEV_READ:
      process_read(nsectors);
      break;
    case BLOCKDEV_WRITE:
      process_write(nsectors);
      break;
    }
  }

  return 0;
}
