# Architecture Overview

## Layered Design

GLDraw now uses a canvas-centered editor architecture:

```text
Application
    -> PlatformWindow
    -> Workspace
        -> Document
        -> CanvasView
        -> ToolController
        -> UiSystem
        -> RenderSystem
```

## Responsibilities

### Application

- boots subsystems
- owns the main loop
- connects GLFW callbacks to the workspace

### PlatformWindow

- creates the GLFW window and OpenGL context
- pumps platform events
- exposes the framebuffer size and swap

### Workspace

- groups the active document, canvas, tools, and UI
- stores the current document path and saved revision marker
- acts as the editor session root

### Document

- owns `GraphicObject` instances
- owns the selection set
- stores object data in world coordinates
- persists to a JSON document format

### CanvasView

- owns viewport/camera state (single canonical `viewport_state`) and background
- converts between screen and world space
- performs picking in canvas space

### ToolController

- owns tool instances
- tracks the active tool
- routes pointer, key, and wheel events to that tool

### RenderSystem

- maps world geometry to screen coordinates through the canvas
- frame-batches compatible line primitives (grid + object strokes + overlay)
- tracks per-frame draw/upload counters for diagnostics

### UiSystem

- builds toolbar, inspector, and status bar
- blocks canvas interaction while the pointer is over UI
- owns authoritative UI layout hit bounds via `layout_snapshot`
- supports theme switching and hot reload with failure-safe fallback behavior

## Key Decisions

1. Document data and canvas state are separate.
2. Tools mutate the document through canvas-aware events.
3. Rendering is stateless with respect to editing logic.
4. Stable object identity uses `ObjectId`, not raw selection pointers.
5. Old direct-coupled `ShapeManager` and `ToolManager` layers were removed.
6. History is hybrid: full snapshots for broad edits + targeted transactions for high-frequency edits.
