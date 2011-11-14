import os
import ctypes

binpath = os.path.dirname(os.path.abspath(__file__))
handle = ctypes.CDLL(binpath + "/../lib/libfesvr.so")

handle.new_htif_isasim.restype = ctypes.c_void_p
handle.new_htif_isasim.argtypes = [ctypes.c_int, ctypes.c_int]

handle.new_htif_csim.restype = ctypes.c_void_p
handle.new_htif_csim.argtypes = [ctypes.c_int, ctypes.c_int]

handle.new_htif_rs232.restype = ctypes.c_void_p
handle.new_htif_rs232.argtypes = [ctypes.c_char_p]

handle.new_htif_eth.restype = ctypes.c_void_p
handle.new_htif_eth.argtypes = [ctypes.c_char_p, ctypes.c_int]

handle.new_memif.restype = ctypes.c_void_p
handle.new_memif.argtypes = [ctypes.c_void_p]

handle.load_elf.restype = None
handle.load_elf.argtypes = [ctypes.c_char_p, ctypes.c_void_p]

handle.htif_start.restype = None
handle.htif_start.argtypes = [ctypes.c_void_p, ctypes.c_int]

handle.htif_stop.restype = None
handle.htif_stop.argtypes = [ctypes.c_void_p, ctypes.c_int]

handle.htif_read_cr_until_change.restype = ctypes.c_int64
handle.htif_read_cr_until_change.argtypes = [ctypes.c_void_p, ctypes.c_int]

handle.htif_read_cr.restype = ctypes.c_int64
handle.htif_read_cr.argtypes = [ctypes.c_void_p, ctypes.c_int]

handle.htif_write_cr.restype = None
handle.htif_write_cr.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int64]

handle.frontend_syscall.restype = ctypes.c_int
handle.frontend_syscall.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int64]

handle.mainvars_argc.restype = None
handle.mainvars_argc.argtypes = [ctypes.c_int]

handle.mainvars_argv.restype = None
handle.mainvars_argv.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p]
