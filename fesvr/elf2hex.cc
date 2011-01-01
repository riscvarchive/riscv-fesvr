#include <iostream>
#include "htif_hexwriter.h"
#include "memif.h"
#include "elf.h"

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <width> <elf_file>" << std::endl;
    return 1;
  }

  unsigned width = atoi(argv[1]);
  if(width < 8 || (width & (width-1)))
  {
    std::cerr << "width must be at least 8 and a power of 2" << std::endl;
    return 1;
  }

  htif_hexwriter_t htif(width);
  memif_t memif(&htif);
  load_elf(argv[2],&memif);
  std::cout << htif;

  return 0;
}
