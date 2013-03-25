// See LICENSE for license details.

#include "htif_zedboard.h"

int main(int argc, char** argv)
{
  std::vector<std::string> args(argv + 1, argv + argc);
  htif_zedboard_t htif(args);
  return htif.run();
}
