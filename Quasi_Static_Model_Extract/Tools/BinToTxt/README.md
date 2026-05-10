# BinToTxt

Standalone converter for the PEEC matrix binary files written by this project.

Binary format:

```text
PEECMAT1
uint64 rows
uint64 cols
double data[rows][cols]
```

Build from a Visual Studio Developer Command Prompt:

```bat
cd Tools\BinToTxt
build.bat
```

Or build with MinGW/MSYS2 g++:

```bat
cd Tools\BinToTxt
build_gcc.bat
```

Usage:

```bat
BinToTxt.exe ..\..\Data\Output\LL_OO.bin
BinToTxt.exe ..\..\Data\Output\PP_OO.bin ..\..\Data\Output\PP_OO.txt
```

If the output path is omitted, the tool writes a `.txt` file next to the input file.
