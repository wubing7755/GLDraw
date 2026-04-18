GLDraw
======================================

A canvas-oriented OpenGL drawing editor in C11.

Build & Run
-----------

Linux / macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/GLDraw
```

Windows (MinGW / MSYS2):

```sh
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
./build-mingw/bin/GLDraw.exe
```

Windows (Visual Studio 2022):

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
./build/bin/Release/GLDraw.exe
```

Or use the convenience script:

```sh
./build.sh          # Release build
./build.sh debug    # Debug build
./build.sh clean    # Clean artifacts
```

Controls
--------

| Key | Action |
|-----|--------|
| V | Select tool |
| H | Hand / pan tool |
| L | Line tool |
| R | Rectangle tool |
| E | Ellipse tool |
| Shift+Click | Toggle selection |
| Ctrl+Z | Undo |
| Ctrl+Y / Ctrl+Shift+Z | Redo |
| Ctrl+S | Save document |
| Ctrl+O | Load document |
| Delete / Backspace | Delete selection |
| Mouse Wheel | Zoom at cursor |
| Esc | Clear tool state |

Documentation
-------------

Full documentation: [doc/wiki/](doc/wiki/)

License: MIT
