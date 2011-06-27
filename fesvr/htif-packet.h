#ifndef HTIF_MAX_DATA_SIZE
# error "You fool!  Define your HTIF_MAX_DATA_SIZE!"
#endif

#include <stdexcept>

enum
{
  HTIF_CMD_READ_MEM,
  HTIF_CMD_WRITE_MEM,
  HTIF_CMD_READ_CONTROL_REG,
  HTIF_CMD_WRITE_CONTROL_REG,
  HTIF_CMD_START,
  HTIF_CMD_STOP,
  HTIF_CMD_ACK,
  HTIF_CMD_NACK,
};

struct packet_t
{
  uint16_t cmd;
  uint16_t seqno;
  uint32_t data_size;
  uint64_t addr;
  uint8_t  data[HTIF_MAX_DATA_SIZE];
};

class packet_error : public std::runtime_error
{
public:
  packet_error(const std::string& s) : std::runtime_error(s) {}
};
class bad_seqno_error : public packet_error
{
public:
  bad_seqno_error() : packet_error("bad seqno") {}
};
class io_error : public packet_error
{
public:
  io_error(const std::string& s) : packet_error(s) {}
};

