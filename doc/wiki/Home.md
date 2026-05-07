# GLDraw Wiki

**GLDraw** is a canvas-centered OpenGL drawing editor built with C11, GLFW, GLAD, and Nuklear.

## Architecture

The runtime is organized around an application shell plus a layered workspace:

- `application.c` owns bootstrap, GLFW callbacks, and frame orchestration
- `Workspace` splits into `EditorCore`, `EditorSession`, and `EditorServices`
- `Document` owns objects, layers, IDs, and spatial queries
- `CommandExecutor` owns undo/redo and transactions
- `ToolController` is registry-backed and focused on active-tool runtime
- `UiSystem` is split into runtime, theme, layout, dialog, chrome, and inspector modules
- Built-in objects and tools are registered through app-level manifests

## Design Goals

- Graphic objects live in world coordinates inside `Document`
- `CanvasView` converts between world and screen space
- Tools translate pointer input into document changes through canvas-aware events
- Stable object identity via `ObjectId` (not raw pointers)
- Rendering is stateless with respect to editing logic

## Technology Stack

| Component | Technology |
|---|---|
| Graphics API | OpenGL 3.3 Core Profile |
| Window / Input | GLFW 3.3.9 |
| OpenGL Loader | GLAD |
| UI | Nuklear |
| Build System | CMake 3.15+ |
| Language | C11 |

## Where to Start

1. [Getting Started](getting-started.md) — build and run the application
2. [Architecture](architecture.md) — high-level runtime structure and current boundaries
3. [Core Systems Overview](core-systems-overview.md) — responsibilities of document, commands, tools, UI, and render
4. [Data Flow](data-flow.md) — understand how events become editor actions and renders
5. [Extending](extending.md) — add new object types or tools
6. [Layering Plan](architecture-layering-plan.md) — historical refactor roadmap and target boundaries
7. [Controls](../../README.md#controls) — keyboard and mouse shortcuts
