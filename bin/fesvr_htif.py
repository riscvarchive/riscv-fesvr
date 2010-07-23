import os
import fcntl
import fesvr_pylink

class htif_isasim_t:

  def __init__(self):
    fhostr, fhostw = os.pipe()
    thostr, thostw = os.pipe()

    fcntl.fcntl(fhostr, fcntl.F_SETFL, fcntl.fcntl(fhostr, fcntl.F_GETFL) & ~os.O_NONBLOCK)
    fcntl.fcntl(thostr, fcntl.F_SETFL, fcntl.fcntl(thostr, fcntl.F_GETFL) & ~os.O_NONBLOCK)

    try:
      h = os.fork()
    except OSError:
      assert 0, "fork() error"

    if h == 0:
      os.execvp('riscv-isa-run', ['riscv-isa-run', '-f'+str(fhostr), '-t'+str(thostw)])
    else:
      os.close(fhostr)
      os.close(thostw)

    self.cinst = fesvr_pylink.handle.new_htif_isasim(thostr, fhostw)
