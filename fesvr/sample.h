#ifndef __SAMPLE_H
#define __SAMPLE_H

#include <vector>
#include <ostream>

enum SAMPLE_CMD {
  FIN, STEP, POKE, EXPECT, WRITE, READ,
};

class sample_t {
public:
  sample_t(size_t size) { snap = new char[size]; }
  ~sample_t() { delete[] snap; }
  char* snap_pos(size_t offset) { return snap + offset; }
  std::ostream& dump(std::ostream& os) const;

  static void add_mapping(std::string signal, size_t width) {
    signals.push_back(signal);
    widths.push_back(width);
  }
 
private:
  char* snap;
  static std::vector<std::string> signals;
  static std::vector<size_t> widths;
};

#endif // __SAMPLE_H
