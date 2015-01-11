#include "sample.h"
#include <string.h>

std::vector<std::string> sample_t::signals = std::vector<std::string> ();
std::vector<size_t> sample_t::widths = std::vector<size_t> ();

std::ostream& sample_t::dump(std::ostream& os) const {
  size_t offset = 0;
  for (size_t i = 0 ; i < signals.size() ; i++) {
    std::string signal = signals[i];
    size_t width = widths[i];
    if (signal != "null") {
      os << std::dec << POKE << " ";
      os << signal << " ";
      os << std::hex; 

      char *bin = new char[width];
      uint64_t value = 0; // TODO: more than 32 bits?
      memcpy(bin, snap+offset, width);
      for (size_t i = 0 ; i < width ; i++) {
        value = (value << 1) | (bin[i] - '0'); // index?
      }
      os << value;
      /*
      uint64_t *values = new uint64_t[width/64];
      for (size_t i = 0 ; i < width ; i += 64) {
        size_t size = width - i < 64 ? width - i : 64;
        memcpy(bin, snap+offset+i+size, size); 
        os << value;
      }
      */
      delete[] bin;
      os << "\n";
    }
    offset += width;
  }
  return os;
}

// operator overloading
std::ostream& operator<<( std::ostream &os, const sample_t& sample) {
  return sample.dump(os);
}
