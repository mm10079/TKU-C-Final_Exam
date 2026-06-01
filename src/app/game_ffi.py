import ctypes
import os
import sys
from pathlib import Path


def _lib_name():
    if sys.platform.startswith('win'):
        return 'game.dll'
    elif sys.platform.startswith('darwin'):
        return 'libgame.dylib'
    else:
        return 'libgame.so'

_here = Path(__file__).resolve().parent
_libpath = _here / 'core' / _lib_name()
_libpath = str(_libpath.resolve())

try:
    _lib = ctypes.CDLL(_libpath)
except OSError as e:
    _lib = None
    _load_error = str(e)
else:
    _load_error = None

if _lib is not None:
    _lib.game_init.restype = ctypes.c_void_p
    _lib.game_left_click.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
    _lib.game_right_click.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
    _lib.game_undo.argtypes = [ctypes.c_void_p]
    _lib.game_get_state.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]
    _lib.game_get_state.restype = ctypes.c_int
    _lib.game_free.argtypes = [ctypes.c_void_p]

class GameFFI:
    def __init__(self):
        self.h = None
        self.difficulty = ""

    def init(self, rows, cols, bomb_count, difficulty='custom'):
        self.difficulty = difficulty
        if _lib is None:
            raise RuntimeError('game library not found at ' + _libpath + (f': {_load_error}' if _load_error else ''))
        self.h = _lib.game_init(ctypes.c_int(rows), ctypes.c_int(cols), ctypes.c_int(bomb_count))
        return self.h

    def left_click(self, r, c):
        if not self.h:
            return -1
        return _lib.game_left_click(self.h, int(r), int(c))

    def right_click(self, r, c):
        if not self.h:
            return -1
        return _lib.game_right_click(self.h, int(r), int(c))

    def undo(self):
        if not self.h:
            return -1
        return _lib.game_undo(self.h)

    def get_state(self):
        if not self.h:
            return '{}'
        buf = ctypes.create_string_buffer(65536)
        _lib.game_get_state(self.h, buf, ctypes.c_size_t(len(buf)))
        return buf.value.decode('utf-8')

    def free(self):
        if not self.h:
            return
        _lib.game_free(self.h)
        self.h = None

_ffi = GameFFI()

def get_ffi():
    return _ffi
