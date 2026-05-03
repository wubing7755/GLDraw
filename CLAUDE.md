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

GLDraw is a canvas-oriented OpenGL drawing editor in C11. The central hub is **Workspace** (`include/app/workspace.h`), which contains:

```
Workspace → EditorCore (Document + CommandExecutor + CanvasView + ToolController)
          → EditorSession (Keymap, Layout, Selection, Clipboard)
          → EditorServices (save/load/export callbacks)
```

### Core Systems

| System | Purpose |
|--------|---------|
| **document** | Owns dynamic `GraphicObject` array, layer system, spatial index, revision tracking |
| **model** | `SelectionSet` (dynamic), `EditorSession` state |
| **canvas** | Viewport, zoom/pan, world↔screen coordinate transforms |
| **tools** | Routes pointer/key events to active tool via `ToolDescriptor` |
| **commands** | `CommandExecutor` with undo/redo, transactions, memory budgeting |
| **render** | `RenderDevice` abstraction, draw-list build, GL backend via factory |
| **ui** | Nuklear toolbar, menu, inspector panel, status bar, dialogs |

### Key Patterns

**Object Descriptor Pattern** — `GraphicObjectDescriptor` (`include/document/object.h`) lets object types (line, rect, ellipse, extensions) define their own create/clone/destroy/translate/get_bounds/hit_test/serialize/deserialize methods. New shape types register via `register_object_type()` without modifying core files.

**Tool Descriptor Pattern** — Tools (`include/tools/tool.h`) register `ToolDescriptor` with create/activate/pointer/key handlers. Commands and menu items for tools are dynamically generated from the `ToolRegistry`. Extensions register tools the same way as built-ins.

**Command-Based Undo** — All document mutations go through `CommandExecutor` (`include/commands/command.h`). Commands implement execute/undo/redo/merge/destroy. The executor supports transactions with auto-rollback on failure. No external snapshot-based undo exists.

### Data Flow

```
GLFW events → application.c callbacks → input_router → command_registry_execute
           → tool_controller_* → command_executor_execute → Document mutation
           → render_system_draw → canvas_drawlist → canvas_renderer → OpenGL frame
```

### Extension System

- **Object types**: Register via `register_object_type()` in `extension_loader.c`. Core `object.c` never imports extension headers.
- **Tools**: Register via `register_tool()`. Commands, shortcuts, and menu items are derived from `ToolDescriptor` metadata.
- **Serialization**: Extensions serialize/deserialize via `GraphicObjectDescriptor` vtable — no core change needed.

JSON persistence via `document_save_json()` / `document_load_json()`.

---

## Entry Points

- `src/main.c` — single line calling `app_run()`
- `src/app/application.c` — bootstrap, window, main loop, dependency injection
- `src/app/extension_loader.c` — extension registration assembly point
- `src/commands/command.c` — command types, executor, transactions
- `src/render/render_system.c` — draw-list build, render submission
- `src/tools/tool_controller.c` — tool routing and built-in tool implementations

## Documentation

Detailed architecture docs in `doc/wiki/`:
- `architecture.md`, `core-systems-overview.md` — high-level design
- `data-flow.md` — main loop, event, render flows
- `extending.md` — guide for adding new object types and tools
