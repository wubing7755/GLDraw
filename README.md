GLDraw
======================================

[![C Version](https://img.shields.io/badge/C-C11-blue)](https://en.wikipedia.org/wiki/C_(programming_language))
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![CN](https://img.shields.io/badge/中文-red)](./doc/README-zh.md)

GLDraw is now a canvas-oriented OpenGL drawing editor. The project has been restructured around the same separation used by common graphics tools:

- `Window` hosts the application shell and platform events
- `Document` owns graphic objects and selection state
- `CanvasView` owns zoom, pan, viewport, and coordinate transforms
- `ToolController` routes input into editing tools
- `RenderSystem` draws the document and tool overlays
- `UiSystem` hosts the toolbar, inspector, and status bar

## Current Features

- Single-window, single-document editor
- Infinite-style canvas with zoom at cursor and pan
- Line, rectangle, and ellipse drawing tools
- Selection, shift-multi-select, drag move, delete selection
- Undo / redo for create, move, delete, and inspector edits
- JSON document save / load using the current path or `document.json`
- Inspector for stroke color, width, and basic geometry
- Grid and origin axes rendering
- Modular C11 codebase with GLFW, GLAD, and Nuklear

## Quick Start

Windows with MinGW / MSYS2:

```sh
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
./build-mingw/bin/GLDraw.exe
```

Windows with Visual Studio 2022:

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
./build/bin/Release/GLDraw.exe
```

## Controls

- `V`: Select tool
- `H`: Hand / pan tool
- `L`: Line tool
- `R`: Rectangle tool
- `E`: Ellipse tool
- `Shift+Click`: Toggle selection
- `Ctrl+Z`: Undo
- `Ctrl+Y` or `Ctrl+Shift+Z`: Redo
- `Ctrl+S`: Save document JSON to the current path
- `Ctrl+O`: Load document JSON from the current path
- `Delete` / `Backspace`: Delete current selection
- `Mouse Wheel`: Zoom at cursor
- `Esc`: Clear tool state, or close window when already in select mode

## Project Structure

```text
GLDraw/
├── include/
│   ├── app/        # Application bootstrap and workspace wiring
│   ├── base/       # Shared math, logging, and primitive types
│   ├── canvas/     # Canvas view and coordinate transforms
│   ├── document/   # Graphic objects, document, selection
│   ├── platform/   # GLFW window abstraction
│   ├── render/     # OpenGL renderer
│   ├── tools/      # Tool protocol and tool controller
│   ├── ui/         # Nuklear UI integration
│   ├── glad/
│   ├── KHR/
│   └── nuklear/
├── src/
│   ├── app/
│   ├── canvas/
│   ├── document/
│   ├── platform/
│   ├── render/
│   ├── tools/
│   ├── ui/
│   ├── glad.c
│   └── main.c
└── shaders/
```

## Architecture Notes

The key relationship in the new design is:

`Window -> Workspace -> Document + CanvasView + ToolController + UiSystem + RenderSystem`

Important boundaries:

- Graphic objects are stored in world coordinates inside `Document`
- `Document` persists as JSON and currently includes the selection set
- `CanvasView` converts between world and screen space
- Tools modify the document only through canvas-aware events
- The renderer consumes document state and a canvas transform, but does not own editing state

## Dependencies

| Dependency | Role | Management |
|---|---|---|
| GLFW 3.3.9 | Windowing and input | Local cached source via CMake |
| GLAD | OpenGL loader | Committed in repo |
| Nuklear | Immediate-mode UI | Header-only local include |

## Documentation

- Wiki Home: [doc/wiki/Home.md](./doc/wiki/Home.md)
- C Contributor Guide (EN): [doc/c-language-must-know-for-gldraw.en.md](./doc/c-language-must-know-for-gldraw.en.md)
- C 贡献者指南 (ZH): [doc/c-language-must-know-for-gldraw.md](./doc/c-language-must-know-for-gldraw.md)

## Status

This is the current baseline of the refactor. The new architecture is in place, the old direct-coupled runtime has been removed from the build, and the editor now supports document history plus JSON persistence. Future work can extend this foundation with:

- layers and grouping
- snapping and guides
- multiple canvases or multiple documents
