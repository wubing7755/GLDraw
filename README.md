GLDraw
======================================

[![C Version](https://img.shields.io/badge/C-C11-blue)](https://en.wikipedia.org/wiki/C_(programming_language))
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![CN](https://img.shields.io/badge/中文-red)](./doc/README-zh.md)



Contents:

     I.   Overview
     II.  Quick Start
    III. Project Structure
     IV.  Build Instructions
      V.  Debugging
     VI.  Technical Overview
     VII. Dependencies



I. Overview

GLDraw is a minimal OpenGL 3.3 example built with GLFW and Nuklear GUI.

Key features:

- Clean and modular code structure
- OpenGL 3.3 Core Profile
- Cross-platform support (Windows, Linux, macOS)
- Nuklear immediate-mode GUI for runtime control
- CMake-based dependency management



II. Quick Start

     $ git clone https://github.com/wubing7755/GLDraw.git
     $ cd GLDraw
     $ mkdir build && cd build
     $ cmake -G "MinGW Makefiles" ..
     $ cmake --build . --parallel

Run:

     $ ./bin/GLDraw



III. Project Structure

     GLDraw/
     ├── CMakeLists.txt            # Build configuration
     ├── LICENSE.txt               # MIT License
     ├── README.md                 # This file
     ├── include/                  # Header files
     │   ├── core/                 # Project headers
     │   └── nuklear/              # Nuklear GUI library (header-only)
     ├── src/                      # Source files
     │   ├── main.c
     │   ├── ...
     │   ├── window.c
     │   └── nuklear_ui.c
     └── shaders/                  # GLSL shaders
         ├── basic.vert
         └── basic.frag



IV. Build Instructions

**Prerequisites:** CMake 3.15+, C11 compiler, Python 3 (for GLAD generation)

Windows (MinGW / MSYS2):

     $ cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -S . -B build
     $ cmake --build build --parallel

Windows (Visual Studio):

     $ cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
     $ cmake --build build --config Release

Linux / macOS:

     $ cmake -S . -B build
     $ cmake --build build --parallel

VSCode:

     Press F5 to debug
     or Ctrl+Shift+P -> Run Task -> CMake: Build



V. Debugging

VSCode launch configurations:

     - GLDraw (Debug)     : Debug mode with breakpoints
     - GLDraw (No Debug)  : Run without debugging



VI. Technical Overview

C Language:

     - Cross-platform conditional compilation
     - Modular architecture (window / renderer / shader / input / ui)

OpenGL:

     - OpenGL 3.3 Core Profile
     - VAO / VBO / EBO for geometry
     - Uniform-based runtime parameter control
     - Event-driven render loop



VII. Dependencies

| Dependency | Version | Source | Management |
|---|---|---|---|
| GLFW | 3.3.9 | github.com/glfw/glfw | CMake FetchContent |
| GLAD | 2.x (generated) | github.com/Dav1dde/glad | Python during CMake configure |
| Nuklear | master | github.com/Immediate-Mode-UI/Nuklear | Local (header-only) |

All third-party dependencies are automatically fetched or generated during CMake configuration.

     If you have any questions or suggestions, feel free to open an issue.

     Contributions are welcome. See the source code for implementation details.
