from game_ffi import get_ffi

ffi = get_ffi()

if __name__ == '__main__':
    h = ffi.init(5, 5, 5)
    print('init ok', bool(h))
    print('state', ffi.get_state())
    print('left click result', ffi.left_click(0, 0))
    print('state after click', ffi.get_state())
    print('undo result', ffi.undo())
    print('state after undo', ffi.get_state())
    ffi.free()
