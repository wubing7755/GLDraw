# GLDraw

A canvas-oriented OpenGL drawing editor written in C11.

## Quick Start

Choose your platform:

| Platform | Script | Modes |
|---|---|---|
| Linux / macOS | `./build.sh` | `release` (default), `debug`, `clean` |
| Windows (MinGW/MSYS2) | `./build.bat` | `release` (default), `debug`, `clean` |

<details>
<summary>Linux / macOS</summary>

```sh
./build.sh           # release
./build.sh debug     # debug
./build.sh clean     # clean
```

Run:

```sh
./build/Release/bin/GLDraw
```
</details>

<details>
<summary>Windows (MinGW/MSYS2, CMD)</summary>

```bat
build.bat            rem release
build.bat debug      rem debug
build.bat clean      rem clean
```

Run:

```bat
build\Release\bin\GLDraw.exe
```
</details>

<details>
<summary>Windows (Visual Studio 2022 x64, manual CMake)</summary>

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Run:

```bat
build\bin\Release\GLDraw.exe
```
</details>

## Build Environment

Choose your platform:

| Platform | Environment |
|---|---|
| Linux / macOS | CMake + C11 compiler + OpenGL dev environment |
| Windows (MinGW/MSYS2) | CMake + MinGW-w64 + OpenGL dev environment |

<details>
<summary>Linux / macOS</summary>

- CMake 3.15+
- C11 compiler (`gcc` or `clang`)
- OpenGL development environment (headers + runtime)
- Build tools for Makefiles (`make`)
- Git + network access on first configure (to fetch GLFW 3.3.9)
- macOS: Xcode Command Line Tools
</details>

<details>
<summary>Windows (MinGW/MSYS2)</summary>

- CMake 3.15+
- MinGW-w64 C11 toolchain (`gcc`)
- `mingw32-make` in `PATH`
- OpenGL development environment (headers + runtime)
- Git + network access on first configure (to fetch GLFW 3.3.9)
</details>

## Core Controls

`V` Select, `H` Pan, `L` Line, `R` Rectangle, `E` Ellipse  
`Ctrl+Z` Undo, `Ctrl+Y`/`Ctrl+Shift+Z` Redo  
`Ctrl+S` Save, `Ctrl+O` Open

## Documentation

- Wiki index: [doc/wiki](doc/wiki/)
- Architecture: [doc/wiki/architecture.md](doc/wiki/architecture.md)
- Extending guide: [doc/wiki/extending.md](doc/wiki/extending.md)

## License

MIT
