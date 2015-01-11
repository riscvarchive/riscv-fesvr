#include "simif_zedboard.h"

int main(int argc, char** argv)
{
  std::vector<std::string> args(argv + 1, argv + argc);
  simif_zedboard_t simif(args);
  return simif.run();
}
