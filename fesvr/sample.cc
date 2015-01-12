#include "sample.h"
#include <string.h>
#include <stdexcept>

std::vector<std::string> sample_t::signals = std::vector<std::string> ();
std::vector<size_t> sample_t::widths = std::vector<size_t> ();

static inline char bin_to_hex (std::string bin) {
  if (bin == "0000") return '0';
  else if (bin == "0001") return '1';
  else if (bin == "0010") return '2';
  else if (bin == "0011") return '3';
  else if (bin == "0100") return '4';
  else if (bin == "0101") return '5';
  else if (bin == "0110") return '6';
  else if (bin == "0111") return '7';
  else if (bin == "1000") return '8';
  else if (bin == "1001") return '9';
  else if (bin == "1010") return 'a';
  else if (bin == "1011") return 'b';
  else if (bin == "1100") return 'c';
  else if (bin == "1101") return 'd';
  else if (bin == "1110") return 'e';
  else if (bin == "1111") return 'f';
  else throw std::runtime_error("given wrong bin number!");
} 

// operator overloading
std::ostream& operator<<(std::ostream &os, const sample_t& sample) {
  size_t offset = 0;
  for (size_t i = 0 ; i < sample.signals.size() ; i++) {
    std::string signal = sample.signals[i];
    size_t width = sample.widths[i];
    if (signal != "null") {
      os << POKE << " " << signal << " ";
      char *buf = new char[width+1];
      memcpy(buf, sample.snap+offset, width);
      buf[width] = '\0';
      std::string bin = buf;
      if (width % 4 > 0) bin.insert(bin.begin(), 4 - (width % 4), '0');
      for (size_t i = 0 ; i < bin.size() ; i += 4) {
        os << bin_to_hex(bin.substr(i, 4));
      }
      os << std::endl;
      bin.clear();
      delete[] buf;
    }
    offset += width;
  }
  return os;
}
