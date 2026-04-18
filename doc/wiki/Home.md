# GLDraw Wiki

**GLDraw** is a canvas-centered OpenGL drawing editor built with C11, GLFW, GLAD, and Nuklear.

## Architecture

The project follows the same split used by common graphics tools:

- `Window` hosts the application shell and platform events
- `Document` stores graphic objects and selection state
- `CanvasView` controls zoom, pan, viewport, and coordinate transforms
- `ToolController` routes user input into editing tools
- `RenderSystem` draws document geometry and overlays
- `UiSystem` builds the editor chrome (toolbar, inspector, status bar)

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

1. [Getting Started](Getting-Started) — build and run the application
2. [Data Flow](Data-Flow) — understand how events become drawings
3. [Extending](Extending) — add new object types or tools
4. [Controls](../../README.md#controls) — keyboard and mouse shortcuts
