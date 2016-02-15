#ifndef __BLOCKDEV_H__
#define __BLOCKDEV_H__

#include <string>
#include <vector>

#define SECTOR_SIZE 512

enum {
  BLOCKDEV_READ,
  BLOCKDEV_WRITE,
};

class blockdev_t
{
 public:
  blockdev_t(const std::vector<std::string>& blockdev_args);
  virtual ~blockdev_t();
  int run(void);

protected:
  virtual uint64_t poll(void) = 0;
  virtual void push(uint64_t val) = 0;

 private:
  int fd;
  size_t max_sector;
  size_t head;
  bool _stop;

  void open_file(const char *fname);
  int read_block(void *data);
  int write_block(const void *data);
  int set_head(uint64_t sector_num);

  void process_read(uint64_t nsectors);
  void process_write(uint64_t nsectors);
};

#endif
