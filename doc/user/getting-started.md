# Getting Started

> Audience: users, new contributors
> Purpose: build, run, and complete a first local session
> Source of truth: `build.sh`, `build.bat`, root `README.md`, CMake configuration
> Last reviewed with code: 2026-05-07
> Related: [controls.md](controls.md), [../architecture/overview.md](../architecture/overview.md)

## Supported Build Paths

Use the repository build scripts when possible.

| Platform | Recommended command | Modes |
|---|---|---|
| Linux / macOS | `./build.sh` | `release` (default), `debug`, `clean` |
| Windows (MinGW/MSYS2, CMD) | `build.bat` | `release` (default), `debug`, `clean` |

Manual CMake is still supported for Linux / macOS and Visual Studio on Windows.

## Quick Choice

| If you are on... | Use this first |
|---|---|
| Linux / macOS | `./build.sh` |
| Windows with MinGW/MSYS2 | `build.bat` |
| Windows with Visual Studio toolchain | manual CMake with `Visual Studio 17 2022` |

## Requirements

| Platform | Required environment |
|---|---|
| Linux / macOS | CMake 3.15+, C11 compiler, OpenGL development environment, Make-compatible build tools |
| Windows (MinGW/MSYS2) | CMake 3.15+, MinGW-w64 C11 toolchain, `mingw32-make`, OpenGL development environment |
| Windows (Visual Studio) | Visual Studio 2022 x64 toolchain, CMake, OpenGL runtime |

Notes:

- GLFW 3.3.9 is fetched by CMake on first configure, so Git and network access are required the first time.
- GLAD is committed in the repository.
- Nuklear is header-only and is included through the source tree.

## Recommended Commands

### Linux / macOS via build script

```sh
./build.sh
./build.sh debug
./build.sh clean
```

Run:

```sh
./build/Release/bin/GLDraw
```

### Windows (MinGW/MSYS2, CMD) via build script

```bat
build.bat
build.bat debug
build.bat clean
```

Run:

```bat
build\Release\bin\GLDraw.exe
```

## Manual CMake Reference

Use manual CMake when you need generator-level control or want the platform-default output layout.

### Linux / macOS via manual CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Run:

```sh
./build/bin/GLDraw
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

## First Local Session

1. Start the application.
2. Press `L`, `R`, or `E` to switch to a shape tool.
3. Left-drag on the canvas to create an object.
4. Press `V` to switch back to the select tool.
5. Click to select one object, or `Shift+Click` to toggle selection membership.
6. Drag selected objects to move them.
7. Use the right-side inspector to change stroke and geometry values.
8. Use the mouse wheel to zoom at the cursor.
9. Press `Ctrl+S` to save or `Ctrl+O` to load.

## Runtime Notes

- Document geometry is stored in world space, not window space.
- The current theme is persisted in `gldraw.settings.json`.
- Document save/load goes through JSON persistence.
- Zoom is clamped in the canvas view and is also accessible through menu actions and shortcuts.

## Troubleshooting

- First configure fails while fetching GLFW:
  Verify Git and network access, then rerun CMake.
- Build works but the app does not launch from the documented path:
  Check whether you used the script output layout (`build/Release/...`) or raw CMake output layout (`build/bin/...`).
- Keyboard shortcuts seem different from the menus:
  See [controls.md](controls.md). Some display strings use menu-friendly forms such as `Ctrl++` while the keymap stores `Ctrl+Plus`.
