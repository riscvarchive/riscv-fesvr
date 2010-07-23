import os
import ctypes

binpath = os.path.dirname(os.path.abspath(__file__))
handle = ctypes.CDLL(binpath + "/../lib/libfesvr.so")
