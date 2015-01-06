#include "simif.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>

simif_t::simif_t(std::vector<std::string> args, uint64_t _max_cycles, bool _trace)
  : t(0), fail_t(-1), pass(true), is_done(false), max_cycles(_max_cycles), trace(_trace),
    qin_num(0), qout_num(0), win_num(0), wout_num(0),
    hostlen(0), addrlen(0), memlen(0), taglen(0), tracelen(0),
    snaplen(0), snaps(NULL)
{
  // Read mapping files
  read_params("TopShim.prm");
  read_io_map("Top.io.map");
  read_chain_map("Top.chain.map");

  // Remove snapshot files before getting started
  open_snap("Top.snap");

  srand(time(NULL));

  // instantiate htif
  htif = new htif_pthread_t(args);
}

simif_t::~simif_t() {
  fclose(snaps);
  delete htif;
}

void simif_t::open_snap(std::string filename) {
  if (snaps != NULL) fclose(snaps);
  snaps = fopen(filename.c_str(), "w");
}

void simif_t::read_params(std::string filename) {
  std::ifstream file(filename.c_str());
  std::string line;
  if (file) {
    while(getline(file, line)) {
      std::string pair = line.substr(1, line.length()-1);
      size_t colon_idx = pair.find(",");
      if (colon_idx < pair.length()) {
        std::string param = pair.substr(0, colon_idx);
        size_t value = std::stoi(pair.substr(colon_idx+1));
        if (param == "HTIF_WIDTH") htiflen = value;
        else if (param == "MIF_ADDR_BITS") addrlen = value;
        else if (param == "MIF_DATA_BITS") memlen = value;
        else if (param == "MIF_TAG_BITS") taglen = value;
        else if (param == "HOST_LEN") hostlen = value;
        else if (param == "CMD_LEN") cmdlen = value;
        else if (param == "TRACE_LEN") tracelen = value;
      }
    }
  } else {
    std::cerr << "Cannot open " << filename << std::endl;
    exit(0);
  }
  file.close();
}

void simif_t::read_io_map(std::string filename) {
  enum IOType { QIN, QOUT, WIN, WOUT };
  IOType iotype = QIN;
  std::ifstream file(filename.c_str());
  std::string line;
  if (file) {
    while (getline(file, line)) {
      std::istringstream iss(line);
      std::string head;
      iss >> head;
      if (head == "QIN:") iotype = QIN;
      else if (head == "QOUT:") iotype = QOUT;
      else if (head == "WIN:") iotype = WIN;
      else if (head == "WOUT:") iotype = WOUT;
      else {
        size_t width;
        iss >> width;
        size_t n = (width-1)/hostlen + 1;
        switch (iotype) {
          case QIN:
            qin_map[head] = std::vector<size_t>();
            for (int i = 0 ; i < n ; i++) {
              qin_map[head].push_back(qin_num);
              qin_num++;
            }
            break;
          case QOUT:
            qout_map[head] = std::vector<size_t>();
            for (int i = 0 ; i < n ; i++) {
              qout_map[head].push_back(qout_num);
              qout_num++;
            }
            break;
          case WIN:
            win_map[head] = std::vector<size_t>();
            for (int i = 0 ; i < n ; i++) {
              win_map[head].push_back(win_num);
              win_num++;
            }
            break;
          case WOUT:
            wout_map[head] = std::vector<size_t>();
            for (int i = 0 ; i < n ; i++) {
              wout_map[head].push_back(wout_num);
              wout_num++;
            }
            break;
          default:
            break;
        }
      }
    }
    for (int i = 0 ; i < win_num ; i++) {
      poke_map[i] = 0;
    }
    for (int i = 0 ; i < qin_num ; i++) {
      pokeq_map[i] = std::queue<uint32_t>();
    }
    for (int i = 0 ; i < qout_num ; i++) {
      peekq_map[i] = std::queue<uint32_t>();
    }
  } else {
    std::cerr << "Cannot open " << filename << std::endl;
    exit(0);
  }
  file.close();
}

void simif_t::read_chain_map(std::string filename) {
  std::ifstream file(filename.c_str());
  std::string line;
  if (file) {
    while(getline(file, line)) {
      std::istringstream iss(line);
      std::string path;
      size_t width;
      iss >> path >> width;
      signals.push_back(path);
      widths.push_back(width);
      snaplen += width;
    }
  } else {
    std::cerr << "Cannot open " << filename << std::endl;
    exit(0);
  }
  file.close();
}

void simif_t::poke_steps(size_t n, bool record) {
  poke(n << (cmdlen+1) | record << cmdlen | STEP_CMD);
}

void simif_t::poke_all() {
  poke(POKE_CMD);
  for (int i = 0 ; i < win_num ; i++) {
    poke(poke_map[i]);
  }
}

void simif_t::peek_all() {
  peek_map.clear();
  poke(PEEK_CMD);
  for (int i = 0 ; i < wout_num ; i++) {
    peek_map[i] = peek();
  }
}

void simif_t::pokeq_all() {
  if (qin_num > 0) poke(POKEQ_CMD);
  for (int i = 0 ; i < qin_num ; i++) {
    uint32_t count = (pokeq_map[i].size() < tracelen) ? pokeq_map[i].size() : tracelen;
    poke(count);
    for (int k = 0 ; k < count ; k++) {
      poke(pokeq_map[i].front());
      pokeq_map[i].pop();
    }
  }
}

void simif_t::peekq_all() {
  if (qout_num > 0) poke(PEEKQ_CMD);
  trace_qout();
}

void simif_t::trace_qout() {
  for (int i = 0 ; i < qout_num ; i++) {
    uint32_t count = peek();
    for (int k = 0 ; k < count ; k++) {
      peekq_map[i].push(peek());
    }
  }
}

void simif_t::peek_trace() {
  poke(TRACE_CMD);
  trace_mem();
}

void simif_t::trace_mem() {
  std::vector<uint64_t> waddr;
  std::vector<uint64_t> wdata;
  uint32_t wcount = peek();
  for (int i = 0 ; i < wcount ; i++) {
    uint64_t addr = 0;
    for (int k = 0 ; k < addrlen ; k += hostlen) {
      addr |= peek() << k;
    }
    waddr.push_back(addr);
  }
  for (int i = 0 ; i < wcount ; i++) {
    uint64_t data = 0;
    for (int k = 0 ; k < memlen ; k += hostlen) {
      data |= peek() << k;
    }
    wdata.push_back(data);
  }
  for (int i = 0 ; i < wcount ; i++) {
    uint64_t addr = waddr[i];
    uint64_t data = wdata[i];
    mem_writes[addr] = data;
  }
  waddr.clear();
  wdata.clear();

  uint32_t rcount = peek();
  for (int i = 0 ; i < rcount ; i++) {
    uint64_t addr = 0;
    for (int k = 0 ; k < addrlen ; k += hostlen) {
      addr = (addr << hostlen) | peek();
    }
    uint64_t tag = 0;
    for (int k = 0 ; k < taglen ; k += hostlen) {
      tag = (tag << hostlen) | peek();
    }
    mem_reads[tag] = addr;
  }
}

static inline char* int_to_bin(uint32_t value, size_t size) {
  char* bin = new char[size];
  for(int i = 0 ; i < size ; i++) {
    bin[i] = ((value >> (size-1-i)) & 0x1) + '0';
  }
  return bin;
}

void simif_t::read_snap(char *snap) {
  for (size_t offset = 0 ; offset < snaplen ; offset += hostlen) {
    char* value = int_to_bin(peek(), hostlen);
    memcpy(snap+offset, value, (offset+hostlen < snaplen) ? hostlen : snaplen-offset);
    delete[] value; 
  }
}

void simif_t::record_io() {
  for (iomap_t::iterator it = win_map.begin() ; it != win_map.end() ; it++) {
    std::string signal = it->first;
    std::vector<size_t> ids = it->second;
    uint32_t data = 0;
    for (int i = 0 ; i < ids.size() ; i++) {
      size_t id = ids[i];
      data = (data << hostlen) | ((poke_map.find(id) != poke_map.end()) ? poke_map[id] : 0);
    } 
    fprintf(snaps, "%d %s %x\n", SNAP_POKE, signal.c_str(), data);
  }
  fprintf(snaps, "%d\n", SNAP_FIN);
}

void simif_t::record_snap(char *snap) {
  size_t offset = 0;
  for (int i = 0 ; i < signals.size() ; i++) {
    std::string signal = signals[i];
    size_t width = widths[i];
    if (signal != "null") {
      char *bin = new char[width];
      uint32_t value = 0; // TODO: more than 32 bits?
      memcpy(bin, snap+offset, width);
      for (int i = 0 ; i < width ; i++) {
        value = (value << 1) | (bin[i] - '0'); // index?
      }
      fprintf(snaps, "%d %s %x\n", SNAP_POKE, signal.c_str(), value);
      delete[] bin;
    }
    offset += width;
  }
}

void simif_t::record_mem() {
  for (map_t::iterator it = mem_writes.begin() ; it != mem_writes.end() ; it++) {
    uint32_t addr = it->first;
    uint32_t data = it->second;
    fprintf(snaps, "%d %x %08x\n", SNAP_WRITE, addr, data);
  }

  for (map_t::iterator it = mem_reads.begin() ; it != mem_reads.end(); it++) {
    uint32_t tag = it->first;
    uint32_t addr = it->second;
    fprintf(snaps, "%d %x %08x\n", SNAP_READ, addr, tag);
  }

  mem_writes.clear();
  mem_reads.clear();
}

void simif_t::step(size_t n, bool record) {
  uint64_t target_t = t + n;
  if (record && t > 0) record_io();
  if (trace) fprintf(stderr, "* STEP %ld -> %ld *\n", n, target_t);

  // poke_all();
  // pokeq_all();
  poke_steps(n, record);
  bool fin = false;
  while (!fin) {
    step_htif();
    if (peek_ready()) {
      uint32_t resp = peek();
      if (resp == RESP_FIN) fin = true;
      else if (resp == RESP_TRACE) trace_mem();
      else if (resp == RESP_PEEKQ) trace_qout();
    }
  }
  char *snap = new char[snaplen];
  if (record) read_snap(snap);
  // peek_all();
  // peekq_all();
  if (record) {
    peek_trace();
    record_snap(snap);
    record_mem();
  }
  delete[] snap;
  t += n;
}

void simif_t::poke(std::string path, uint64_t value) {
  assert(win_map.find(path) != win_map.end());
  std::vector<size_t> ids = win_map[path];
  uint64_t mask = (1 << hostlen) - 1;
  for (int i = 0 ; i < ids.size() ; i++) {
    size_t id = ids[ids.size()-1-i];
    size_t shift = hostlen * i;
    uint32_t data = (value >> shift) & mask;
    poke_map[id] = data;
  }
  if (trace) fprintf(stderr, "* POKE %s %lx *\n", path.c_str(), value);
}

uint64_t simif_t::peek(std::string path) {
  assert(wout_map.find(path) != wout_map.end());
  uint64_t value = 0;
  std::vector<size_t> ids = wout_map[path];
  for (int i = 0 ; i < ids.size() ; i++) {
    size_t id = ids[ids.size()-1-i];
    assert(peek_map.find(id) != peek_map.end());
    value = value << hostlen | peek_map[id];
  }
  if (trace) fprintf(stderr, "* PEEK %s -> %lx\n *", path.c_str(), value);
  return value;
}

void simif_t::pokeq(std::string path, uint64_t value) {
  assert(qin_map.find(path) != qin_map.end());
  std::vector<size_t> ids = qin_map[path];
  uint64_t mask = (1 << hostlen) - 1;
  for (int i = 0 ; i < ids.size() ; i++) {
    size_t id = ids[ids.size()-1-i];
    size_t shift = hostlen * i;
    uint32_t data = (value >> shift) & mask;
    assert(pokeq_map.find(id) != pokeq_map.end());
    pokeq_map[id].push(data);
  }
  if (trace) fprintf(stderr, "* POKEQ %s <- %lx *\n", path.c_str(), value);
}

uint64_t simif_t::peekq(std::string path) {
  assert(qout_map.find(path) != qout_map.end());
  std::vector<size_t> ids = qout_map[path];
  uint64_t value = 0;
  for (int i = 0 ; i < ids.size() ; i++) {
    size_t id = ids[ids.size()-1-i];
    assert(peekq_map.find(id) != peekq_map.end());
    value = value << hostlen | peekq_map[id].front();
    peekq_map[id].pop();
  }
  if (trace) fprintf(stderr, "* PEEKQ %s %lx *\n", path.c_str(), value);
  return value;
}

bool simif_t::peekq_valid(std::string path) {
  assert(qout_map.find(path) != qout_map.end());
  std::vector<size_t> ids = qout_map[path];
  bool valid = true;
  for (int i = 0 ; i < ids.size() ; i++) {
    size_t id = ids[ids.size()-1-i];
    assert(peekq_map.find(id) != peekq_map.end());
    valid &= !peekq_map[id].empty();
  }
  return valid;
}

bool simif_t::expect(std::string path, uint64_t expected) {
  uint64_t value = peek(path);
  bool ok = value == expected;
  pass &= ok;
  if (!ok && t < fail_t) fail_t = t;
  if (trace) fprintf(stderr, "* EXPECT %s -> %lx === %lx %s *\n", 
                     path.c_str(), value, expected, ok ? " PASS" : " FAIL");
  return ok;
}

bool simif_t::expect(bool ok, std::string s) {
  pass &= ok;
  if (!ok && fail_t < 0) fail_t = t;
  if (trace) fprintf(stderr, "* %s %s *\n", s.c_str(), ok ? "PASS" : "FAIL");
  return ok;
}

void simif_t::load_mem(std::string filename) {
  std::ifstream file(filename.c_str());
  if (file) { 
    std::string line;
    int i = 0;
    while (std::getline(file, line)) {
      #define parse_nibble(c) ((c) >= 'a' ? (c)-'a'+10 : (c)-'0')
      uint64_t base = (i * line.length()) / 2;
      uint64_t offset = 0;
      for (int k = line.length() - 8 ; k >= 0 ; k -= 8) {
        uint64_t addr = base + offset;
        uint64_t data = 
          (parse_nibble(line[k]) << 28) | (parse_nibble(line[k+1]) << 24) |
          (parse_nibble(line[k+2]) << 20) | (parse_nibble(line[k+3]) << 16) |
          (parse_nibble(line[k+4]) << 12) | (parse_nibble(line[k+5]) << 8) |
          (parse_nibble(line[k+6]) << 4) | parse_nibble(line[k+7]);
        write_mem(base + offset, data);
        offset += 4;
      }
      i += 1;
    }
  } else {
    std::cerr << "Cannot open " << filename << std::endl;
    exit(1);
  }
  file.close();
}

void simif_t::write_mem(uint64_t addr, uint64_t data) {
  poke((1 << cmdlen) | MEM_CMD);
  uint64_t mask = (1<<hostlen)-1;
  for (int i = (addrlen-1)/hostlen+1 ; i > 0 ; i--) {
    poke((addr >> (hostlen * (i-1))) & mask);
  }
  for (int i = (memlen-1)/hostlen+1 ; i > 0 ; i--) {
    poke((data >> (hostlen * (i-1))) & mask);
  }

  mem_writes[addr] = data;
}

uint64_t simif_t::read_mem(uint64_t addr) {
  poke((0 << cmdlen) | MEM_CMD);
  uint64_t mask = (1<<hostlen)-1;
  for (int i = (addrlen-1)/hostlen+1 ; i > 0 ; i--) {
    poke((addr >> (hostlen * (i-1))) & mask);
  }
  uint64_t data = 0;
  for (int i = 0 ; i < (memlen-1)/hostlen+1 ; i ++) {
    data |= peek() << (hostlen * i);
  }
  return data;
}

uint64_t simif_t::rand_next(size_t limit) {
  return rand() % limit;
}

void simif_t::run() {
  poke("Top.io_in_mem_valid", 0);
  poke("Top.io_mem_backup_en", 0);
  poke("Top.io_out_mem_ready", 0);
  while (!htif->done() && cycles() < max_cycles) {
    step(100, false);
    size_t size = htiflen / 8;
    uint64_t buf;
    while (to_htif.size() >= size) {
      std::copy(to_htif.begin(), to_htif.begin() + size, &buf);
      htif->send(&buf, size);
      to_htif.erase(to_htif.begin(), to_htif.begin() + size);    
    }
    while (htif->recv_nonblocking(&buf, size)) {
      from_htif.insert(from_htif.end(), (const char*)&buf, (const char*)&buf + size);
    }
  }

  if (htif->exit_code()) {
    fprintf(stderr, "*** FAILED *** (code = %d) after %lld cycles\n", htif->exit_code(), (long long)cycles());
  } else if (cycles() == max_cycles) {
    fprintf(stderr, "*** FAILED *** (timeout) after %lld cycles\n", (long long)cycles());
  } else {
    fprintf(stderr, "*** PASSED *** after %lld cycles\n", (long long)cycles());
  }

  is_done = true;
}

