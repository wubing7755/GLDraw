# Getting Started

## Requirements

- CMake 3.15+
- A C11 compiler
- OpenGL 3.3 capable driver

## Build

### Windows with MinGW / MSYS2

```sh
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
```

Run:

```sh
./build-mingw/bin/GLDraw.exe
```

### Windows with Visual Studio 2022

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Run:

```sh
./build/bin/Release/GLDraw.exe
```

## First Session

1. Start the application.
2. Press `L`, `R`, or `E` to choose a drawing tool.
3. Drag on the canvas to create an object.
4. Press `V` to return to the select tool.
5. Click to select, `Shift+Click` to multi-select.
6. Drag selected objects to move them.
7. Use the inspector to edit stroke color, width, and geometry.
8. Use the mouse wheel to zoom at the cursor.
9. Press `Ctrl+S` to save to `document.json` or the current path.
10. Press `Ctrl+O` to reload the current document JSON.

## Runtime Notes

- Shader paths are resolved relative to the executable output directory.
- The top toolbar, right inspector, and bottom status bar are handled by Nuklear.
- The canvas stores world-space data independently from the window size.
- Document persistence currently stores document data only, not the canvas view session.
