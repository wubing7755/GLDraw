# Getting Started

## Build

Recommended path:

- Linux / macOS: `./build.sh`
- Windows (MinGW/MSYS2): `./build.bat`

### Linux / macOS via build script

```sh
./build.sh          # Release build
./build.sh debug    # Debug build
./build.sh clean    # Clean artifacts
```

Run:

```sh
./build/Release/bin/GLDraw
```

### Linux / macOS via manual CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Run:

```sh
./build/bin/GLDraw
```

### Windows (MinGW/MSYS2, CMD) via build script

```bat
build.bat           rem Release build
build.bat debug     rem Debug build
build.bat clean     rem Clean artifacts
```

Run:

```bat
build\Release\bin\GLDraw.exe
```

### Windows (Visual Studio 2022 x64) via manual CMake

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Run:

```bat
build\bin\Release\GLDraw.exe
```

For platform requirements and related links, see the main [README.md](../../README.md).

## First Session

1. Start the application.
2. Press `L`, `R`, or `E` to choose a drawing tool.
3. Drag on the canvas to create an object.
4. Press `V` to switch to the select tool.
5. Click to select, `Shift+Click` to multi-select.
6. Drag selected objects to move them.
7. Use the inspector (right panel) to edit stroke color, width, and geometry.
8. Use the mouse wheel to zoom at the cursor.
9. Press `Ctrl+S` to save, `Ctrl+O` to load.

## Runtime Notes

- The canvas stores world-space data independently from the window size.
- Document persistence saves to `document.json` or the current path.
- Theme selection is persisted in `gldraw.settings.json`.
