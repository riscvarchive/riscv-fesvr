#include "sample.h"
#include <algorithm>
#include <stdexcept>
#include <bitset>
#include <string.h>
#include <assert.h>

std::vector<std::string> sample_t::signals = std::vector<std::string> ();
std::vector<size_t> sample_t::widths = std::vector<size_t> ();

bool sample_cmp (sample_t *a, sample_t *b) {
  return a->t < b->t;
}

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

sample_t::sample_t(size_t t_, std::string snap): t(t_) {
  size_t start = 0;
  for (size_t i = 0 ; i < signals.size() ; i++) {
    std::string &signal = signals[i];
    size_t &width = widths[i];
assert(width <= 64); // TODO: remove assert here
    if (signal != "null") {
      std::bitset<64> value(snap.substr(start, width));
      add_cmd(new poke_t(signal, value.to_ullong()));
    }
    start += width;
  }
}

sample_t::~sample_t() {
  for (size_t i = 0 ; i < cmds.size() ; i++) {
    delete cmds[i];
  }
  cmds.clear();
  mem.clear();
}

// Memory merge
sample_t& sample_t::operator+=(const sample_t& that) {
  for (mem_t::const_iterator it = that.mem.begin() ; it != that.mem.end() ; it++) {
    uint64_t addr = it->first;
    uint64_t data = it->second;
    mem[addr] = data;
  }
  return *this;
}

void sample_t::dump(std::ostream &os, std::vector<sample_t*> &samples) {
  std::sort(samples.begin(), samples.end(), sample_cmp);
  for (size_t i = 0 ; i < samples.size() ; i++) {
    os << *samples[i];
  }
}

std::ostream& operator<<(std::ostream &os, const sample_t& sample) {
  for (size_t i = 0 ; i < sample.cmds.size() ; i++) {
    sample_inst_t *cmd = sample.cmds[i];
    os << *cmd;
  }
  for (mem_t::const_iterator it = sample.mem.begin() ; it != sample.mem.end() ; it++) {
    uint64_t addr = it->first;
    uint64_t data = it->second;
    os << WRITE << std::hex << " " << addr << " " << data << std::dec << std::endl;
  }
  os << FIN << std::endl;
  return os;
}

