import os
import sys
import signal
import fcntl
import fesvr_pylink

class htif_isasim_t:

  def __init__(self, args):
    fhostr, fhostw = os.pipe()
    thostr, thostw = os.pipe()

    fcntl.fcntl(fhostr, fcntl.F_SETFL, fcntl.fcntl(fhostr, fcntl.F_GETFL) & ~os.O_NONBLOCK)
    fcntl.fcntl(thostr, fcntl.F_SETFL, fcntl.fcntl(thostr, fcntl.F_GETFL) & ~os.O_NONBLOCK)

    try:
      pid = os.fork()
    except OSError:
      assert 0, "fork() error"

    if pid == 0:
      os.execvp('riscv-isa-run', ['riscv-isa-run'] + args + ['-f'+str(fhostr), '-t'+str(thostw)])
    else:
      os.close(fhostr)
      os.close(thostw)

    self.pid = pid
    self.cinst = fesvr_pylink.handle.new_htif_isasim(thostr, fhostw)

  def kill(self):
    os.kill(self.pid, signal.SIGKILL)

class htif_rtlsim_t:

  def __init__(self, args):
    fhostr, fhostw = os.pipe()
    thostr, thostw = os.pipe()

    fcntl.fcntl(fhostr, fcntl.F_SETFL, fcntl.fcntl(fhostr, fcntl.F_GETFL) & ~os.O_NONBLOCK)
    fcntl.fcntl(thostr, fcntl.F_SETFL, fcntl.fcntl(thostr, fcntl.F_GETFL) & ~os.O_NONBLOCK)

    try:
      pid = os.fork()
    except OSError:
      assert 0, "fork() error"

    if pid == 0:
      os.execvp('./simv', ['./simv'] + args + ['+fromhost='+str(fhostr), '+tohost='+str(thostw)])
    else:
      os.close(fhostr)
      os.close(thostw)

    self.pid = pid
    self.cinst = fesvr_pylink.handle.new_htif_rtlsim(thostr, fhostw)

  def kill(self):
    os.kill(self.pid, signal.SIGKILL)

class htif_fpga_rtlsim_t:

  def __init__(self, args):
    thost = "/tmp/thost"+str(os.getpid())
    fhost = "/tmp/fhost"+str(os.getpid())

    self.thost = thost
    self.fhost = fhost
    self.thostr = -1
    self.fhostw = -1
    self.pid = os.getpid()

    os.mkfifo(thost);
    os.mkfifo(fhost);


    try:
      pid = os.fork()
    except OSError:
      assert 0, "fork() error"

    if pid == 0:
      args = ['work.glbl','work.sim_top']+args
      args = ['-sv_lib','../../lib/disasm-modelsim']+args
      args = ['-sv_lib','../../lib/htif-modelsim']+args
      args = ['-vopt','-voptargs=-O5']+args
      #args = ['-novopt']+args
      args = ['-L',os.environ['XILINX']+'/verilog/mti_se/6.6a/lin64/unisims_ver']+args
      args = ['-L',os.environ['XILINX']+'/verilog/mti_se/6.6a/lin64/secureip']+args
      args = ['-L','work']+args
      os.execvp('vsim', ['vsim'] + args + ['+fromhost='+fhost, '+tohost='+thost])

    try:
      thostr = os.open(thost,os.O_RDONLY)
      self.thostr = thostr
      fhostw = os.open(fhost,os.O_WRONLY)
      self.fhostw = fhostw
      fcntl.fcntl(thostr, fcntl.F_SETFL, fcntl.fcntl(thostr, fcntl.F_GETFL) & ~os.O_NONBLOCK)
      fcntl.fcntl(fhostw, fcntl.F_SETFL, fcntl.fcntl(fhostw, fcntl.F_GETFL) & ~os.O_NONBLOCK)
    except:
      self.kill()

    self.cinst = fesvr_pylink.handle.new_htif_rtlsim(thostr, fhostw)

  def kill(self):
    if self.thostr != -1:
      os.close(self.thostr)
    if self.fhostw != -1:
      os.close(self.fhostw)

    os.unlink(self.thost)
    os.unlink(self.fhost)

    os.kill(self.pid, signal.SIGKILL)

class htif_rs232_t:

  def __init__(self, args):
    self.pid = os.getpid()

    self.cinst = fesvr_pylink.handle.new_htif_rs232("/dev/ttyUSB0")

  def kill(self):
    os.kill(self.pid, signal.SIGKILL)
