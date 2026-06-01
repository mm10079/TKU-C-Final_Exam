# Minesweeper Project (C core + Flask UI)

Prerequisites:
- C compiler (GCC or MSVC)
- Python 3.8+
- Matching architecture between Python and the compiled C DLL (64-bit Python needs a 64-bit DLL)

Build C library on Windows with MinGW (32-bit GCC):

```powershell
gcc -shared -o src/app/core/game.dll src/app/core/game.c
```

If you have a 64-bit compiler, build a 64-bit DLL for 64-bit Python:

```powershell
gcc -m64 -shared -o src/app/core/game.dll src/app/core/game.c
```

Install Python deps and run Flask:

```powershell
python -m venv .venv
.venv\Scripts\activate
pip install -r src/requirements.txt
python src/app/main.py
```

If the DLL cannot be loaded due to architecture mismatch, install a Python interpreter that matches the DLL bitness or use a matching compiler.
