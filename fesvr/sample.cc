#include "sample.h"
#include <string.h>

std::vector<std::string> sample_t::signals = std::vector<std::string> ();
std::vector<size_t> sample_t::widths = std::vector<size_t> ();

// operator overloading
std::ostream& operator<<(std::ostream &os, const sample_t& sample) {
  size_t offset = 0;
  for (size_t i = 0 ; i < sample.signals.size() ; i++) {
    std::string signal = sample.signals[i];
    size_t width = sample.widths[i];
    if (signal != "null") {
      os << std::dec << POKE << " ";
      os << signal << " ";
      os << std::hex; 

      char *bin = new char[width];
      uint64_t value = 0; // TODO: more than 64 bits?
      memcpy(bin, sample.snap+offset, width);
      for (size_t i = 0 ; i < width ; i++) {
        value = (value << 1) | (bin[i] - '0'); // index?
      }
      os << value;
      os << "\n";
      delete[] bin;
    }
    offset += width;
  }
  return os;
}
