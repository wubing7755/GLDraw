# GLDraw

A canvas-centered OpenGL drawing editor built in C11.

## Quick Start

Use the repository build scripts when possible:

| Platform | Recommended command | Modes |
|---|---|---|
| Linux / macOS | `./build.sh` | `release` (default), `debug`, `clean` |
| Windows (MinGW/MSYS2) | `./build.bat` | `release` (default), `debug`, `clean` |

<details>
<summary>Linux / macOS via build script</summary>

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
<summary>Windows (MinGW/MSYS2, CMD) via build script</summary>

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
<summary>Linux / macOS via manual CMake</summary>

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Run:

```sh
./build/bin/GLDraw
```
</details>

<details>
<summary>Windows (Visual Studio 2022 x64) via manual CMake</summary>

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Run:

```bat
build\bin\Release\GLDraw.exe
```
</details>

## Build Requirements

GLDraw is built with CMake and C11, and uses GLFW, GLAD, Nuklear, and an OpenGL 3.3 Core Profile context.

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

## Project Overview

- Canvas-centered 2D drawing workflow with line, rectangle, and ellipse tools
- OpenGL rendering pipeline with GLFW windowing, GLAD loading, and Nuklear UI
- Snapshot-based undo/redo plus JSON document save/load
- Tool-driven architecture organized around `Workspace`, `Document`, and `CanvasView`

## Core Controls

`V` Select, `H` Pan, `L` Line, `R` Rectangle, `E` Ellipse  
`Ctrl+Z` Undo, `Ctrl+Y`/`Ctrl+Shift+Z` Redo  
`Ctrl+S` Save, `Ctrl+O` Open

## Documentation

- Getting started: [doc/wiki/getting-started.md](doc/wiki/getting-started.md)
- Wiki index: [doc/wiki](doc/wiki/)
- Architecture: [doc/wiki/architecture.md](doc/wiki/architecture.md)
- Extending guide: [doc/wiki/extending.md](doc/wiki/extending.md)

## License

MIT
