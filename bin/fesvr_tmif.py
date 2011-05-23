import fesvr_pylink

class tmif_t:

  def __init__(self, htif):
    self.htif = htif.cinst
    self.memif = fesvr_pylink.handle.new_memif(self.htif)

  def start(self):
    fesvr_pylink.handle.htif_start(self.htif)

  def stop(self):
    fesvr_pylink.handle.htif_stop(self.htif)

  def run_until_tohost(self, coreid):
    return fesvr_pylink.handle.htif_read_cr(self.htif, coreid, 16)

  def write_fromhost(self, coreid, val):
    fesvr_pylink.handle.htif_write_cr(self.htif, coreid, 17, val)

  def load_elf(self, fn):
    fesvr_pylink.handle.load_elf(fn, self.memif)

  def syscall(self, mm):
    fesvr_pylink.handle.frontend_syscall(self.htif, self.memif, mm)

  def mainvars_argc(self, argc):
    fesvr_pylink.handle.mainvars_argc(argc)

  def mainvars_argv(self, idx, len, arg):
    fesvr_pylink.handle.mainvars_argv(idx, len, arg)
