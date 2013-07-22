#ifndef _TERM_H
#define _TERM_H

class canonical_terminal_t
{
 public:
  static bool empty();
  static char read();
  static void write(char);
};

#endif
