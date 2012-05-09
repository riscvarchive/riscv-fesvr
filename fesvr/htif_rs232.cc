#include "htif_rs232.h"
#include <algorithm>
#include <sys/types.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#define debug(...)
//#define debug(...) fprintf(stderr,__VA_ARGS__)

htif_rs232_t::htif_rs232_t(int ncores, std::vector<char*> args)
  : htif_t(ncores)
{
  char* tty = NULL;
  for (size_t i = 0; i < args.size(); i++)
    if (strncmp(args[i], "+if=", 4) == 0)
      tty = args[i] + 4;
  assert(tty);

  debug("opening %s\n",tty);
  fd = open(tty,O_RDWR|O_NOCTTY);
  assert(fd != -1);

  struct termios tio;
  memset(&tio,0,sizeof(tio));

  tio.c_cflag = B9600;
  tio.c_iflag = IGNPAR;
  tio.c_oflag = 0;
  tio.c_lflag = 0;
  tio.c_cc[VTIME] = 0;
  tio.c_cc[VMIN] = 1; // read in 8 byte chunks

  tcflush(fd, TCIOFLUSH);
  tcsetattr(fd, TCSANOW, &tio);
}

htif_rs232_t::~htif_rs232_t()
{
  close(fd);
}

ssize_t htif_rs232_t::read(void* buf, size_t max_size)
{
  // XXX this is untested!
  ssize_t bytes = sizeof(packet_header_t);
  for (ssize_t i = 0; i < bytes; i++)
  {
    if (::read(fd, (char*)buf + i, 1) != 1)
      return -1;
    if (i == sizeof(packet_header_t)-1)
      bytes += ((packet_header_t*)buf)->data_size;
  }
  return bytes;
}

ssize_t htif_rs232_t::write(const void* buf, size_t size)
{
  return ::write(fd, buf, size);
}
