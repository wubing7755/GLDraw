# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```sh
# Windows (MinGW/MSYS2)
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
./build-mingw/bin/GLDraw.exe

# Windows (Visual Studio 2022)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
./build/bin/Release/GLDraw.exe

# Linux/macOS
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/GLDraw

# Convenience script
./build.sh          # Release build
./build.sh debug    # Debug build
./build.sh clean    # Clean artifacts
```

**Dependencies**: GLFW 3.3.9 (fetched via CMake), GLAD (committed), Nuklear (header-only). Tests run via CTest (Linux/macOS only in CI).

---

## Architecture

GLDraw is a canvas-oriented OpenGL drawing editor in C11. The central hub is **Workspace** (`include/app/workspace.h`), which owns:

```
Workspace → Document + CanvasView + ToolController + DocumentHistory
```

### Core Systems

| System | Purpose |
|--------|---------|
| **document** | Owns `GraphicObject` array (1024 max), selection set, revision tracking |
| **canvas** | Viewport, zoom/pan, world↔screen coordinate transforms |
| **tools** | Routes pointer/key events to active tool via `ToolVTable` |
| **render** | OpenGL drawing: grid, axes, objects, tool overlays |
| **ui** | Nuklear toolbar, inspector panel, status bar |

### Key Patterns

**VTable Polymorphism** — `GraphicObjectVTable` (`include/document/object.h`) lets object types (line, rect, ellipse) define their own hit_test, translate, get_bounds, etc. Adding a new shape type requires implementing this vtable and adding a type constant.

**Tool VTable** — Tools (`include/tools/tool.h`) receive `ToolContext` with pointers to Workspace, Document, etc. Built-in tools: Select(V), Pan(H), Line(L), Rectangle(R), Ellipse(E).

**Snapshot-Based Undo** — `DocumentHistory` (`include/document/history.h`) stores before/after `DocumentSnapshot` objects (up to 128 entries).

### Data Flow

```
GLFW events → application.c callbacks → tool_controller_* → Document/Canvas update
                                                           → render_system_draw → OpenGL frame
```

JSON persistence via `document_save_json()` / `document_load_json()`.

---

## Entry Points

- `src/main.c` — single line calling `app_run()`
- `src/app/application.c` — bootstrap, main loop, GLFW→workspace event wiring
- `src/render/render_system.c` — OpenGL rendering
- `src/tools/tool_controller.c` — tool routing and built-in tool implementations

## Documentation

Detailed architecture docs in `doc/wiki/`:
- `architecture.md`, `core-systems-overview.md` — high-level design
- `data-flow.md` — main loop, event, render flows
- `extending.md` — guide for adding new object types and tools
