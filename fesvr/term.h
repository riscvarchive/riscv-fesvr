#ifndef _TERM_H
#define _TERM_H

#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>

class canonical_terminal_t
{
 public:
  canonical_terminal_t()
   : restore_tios(false)
  {
    if (tcgetattr(0, &old_tios) == 0)
    {
      struct termios new_tios = old_tios;
      new_tios.c_lflag &= ~(ICANON | ECHO);
      if (tcsetattr(0, TCSANOW, &new_tios) == 0)
        restore_tios = true;
    }
  }
  ~canonical_terminal_t()
  {
    if (restore_tios)
      tcsetattr(0, TCSANOW, &old_tios);
  }
  bool empty()
  {
    struct pollfd pfd;
    pfd.fd = 0;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, 0);
    assert(ret >= 0);
    return ret == 0 || !(pfd.revents & POLLIN);
  }
  char read()
  {
    char ch;
    int ret = ::read(0, &ch, 1);
    assert(ret == 1);
    return ch;
  }
 private:
  struct termios old_tios;
  bool restore_tios;
};

#endif
