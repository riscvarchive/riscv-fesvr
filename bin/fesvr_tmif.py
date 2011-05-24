import fesvr_pylink

class tmif_t:

  def __init__(self, htif):
    self.htif = htif.cinst
    self.memif = fesvr_pylink.handle.new_memif(self.htif)

  def start(self, coreid):
    fesvr_pylink.handle.htif_start(self.htif, coreid)

  def stop(self, coreid):
    fesvr_pylink.handle.htif_stop(self.htif, coreid)

  def read_cr_until_change(self, coreid, regidx):
    return fesvr_pylink.handle.htif_read_cr_until_change(self.htif, coreid, regidx)

  def read_cr(self, coreid, regidx):
    return fesvr_pylink.handle.htif_read_cr(self.htif, coreid, regidx)

  def write_cr(self, coreid, regidx, val):
    fesvr_pylink.handle.htif_write_cr(self.htif, coreid, regidx, val)

  def run_until_tohost(self, coreid):
    return self.read_cr_until_change(coreid, 16)

  def write_fromhost(self, coreid, val):
    self.write_cr(coreid, 17, val)

  def load_elf(self, fn):
    fesvr_pylink.handle.load_elf(fn, self.memif)

  def syscall(self, mm):
    fesvr_pylink.handle.frontend_syscall(self.htif, self.memif, mm)

  def mainvars_argc(self, argc):
    fesvr_pylink.handle.mainvars_argc(argc)

  def mainvars_argv(self, idx, len, arg):
    fesvr_pylink.handle.mainvars_argv(idx, len, arg)
