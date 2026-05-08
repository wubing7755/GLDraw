# GLDraw

English | [简体中文](README.zh-CN.md)

A canvas-centered OpenGL drawing editor built in C11.

![Language](https://img.shields.io/badge/language-C11-00599C)
![Build](https://img.shields.io/badge/build-CMake-064F8C)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%203.3-5586A4)
![UI](https://img.shields.io/badge/UI-Nuklear-222222)
![License](https://img.shields.io/badge/license-MIT-green)
[![zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=for-the-badge&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/wubing7755/GLDraw)

![GLDraw preview](assets/preview.png)

Animated placeholder:
![GLDraw demo placeholder](assets/demo.gif)

## Project Status

Active development.

Current focus:

- editor workflow refinement
- extension-system growth
- rendering and UX polish
- deeper test coverage around command and workspace behavior

## Highlights

- Canvas-oriented 2D editing workflow with world-space document geometry
- C11 codebase built on GLFW, GLAD, Nuklear, and OpenGL 3.3 Core Profile
- Command-based undo/redo with merge and transaction support
- Descriptor-driven object and tool extension model
- JSON document persistence and PNG export
- Layer-aware editing with visibility, lock, rename, and reorder flows

## Quick Start

### Linux / macOS

```sh
./build.sh
./build/Release/bin/GLDraw
```

### Windows (MinGW/MSYS2, CMD)

```bat
build.bat
build\Release\bin\GLDraw.exe
```

More build options and platform notes:
[doc/user/getting-started.md](doc/user/getting-started.md)

## Current Features

- Line, rectangle, and ellipse tools
- Select, move, pan, zoom, and zoom-to-fit flows
- Inspector-based property editing
- Layer creation, activation, visibility, lock, rename, and reorder controls
- Save/load through JSON persistence
- Undo/redo through `CommandExecutor`
- PNG export from the current render path

## Controls

`V` Select, `H` Pan, `L` Line, `R` Rectangle, `E` Ellipse  
`Ctrl+Z` Undo, `Ctrl+Y` / `Ctrl+Shift+Z` Redo  
`Ctrl+S` Save, `Ctrl+O` Open

Full controls:
[doc/user/controls.md](doc/user/controls.md)

## Architecture

GLDraw is organized around `Workspace`:

```text
Workspace
  -> EditorCore
  -> EditorSession
  -> EditorServices
```

Key runtime traits:

- `EditorCore` owns `Document`, `CommandExecutor`, `CanvasView`, and `ToolController`
- durable edits flow through commands instead of ad hoc document mutation
- objects and tools are registered through manifests and descriptor metadata
- UI reads workspace/view-model state and emits actions instead of owning editor truth

More:

- [Architecture Overview](doc/architecture/overview.md)
- [Core Systems](doc/architecture/core-systems.md)
- [Data Flow](doc/architecture/data-flow.md)
- [Extension Model](doc/architecture/extension-model.md)

## Documentation

- [Documentation Index](doc/README.md)
- [Getting Started](doc/user/getting-started.md)
- [Controls](doc/user/controls.md)
- [Contributing Overview](doc/contributing/overview.md)
- [GitHub Collaboration Guidelines](doc/contributing/github-collaboration-guidelines.md)
- [GitHub Templates](doc/contributing/github-templates.md)

## Contributing

See:

- [Contributing Overview](doc/contributing/overview.md)
- [C Contributor Guide](doc/contributing/c-contributor-guide.en.md)
- [GitHub Templates](doc/contributing/github-templates.md)

## License

MIT
