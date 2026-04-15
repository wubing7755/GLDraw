# GLDraw Wiki

**GLDraw** is a canvas-centered OpenGL drawing editor built with GLFW, GLAD, and Nuklear.

The project now follows the same split used by common graphics tools:

- `Window` hosts the application shell
- `Document` stores graphic objects and selection
- `CanvasView` controls zoom, pan, viewport, and coordinate transforms
- `ToolController` routes user intent into editing tools
- `RenderSystem` draws document geometry and overlays
- `UiSystem` builds the editor chrome

## Start Here

1. [Introduction](introduction)
2. [Getting Started](getting-started)
3. [Project Structure](project-structure)
4. [Architecture](architecture)

## Contributor Quick Prep

If you plan to contribute code, read the C contributor guide first.  
It summarizes repository-specific C patterns (callbacks, pointers, vtable usage, memory ownership) and common PR pitfalls.

- [C Contributor Guide (EN)](../c-language-must-know-for-gldraw.en.md)
- [C 贡献者指南 (ZH)](../c-language-must-know-for-gldraw.md)

## Core Systems

- [Overview](core-systems-overview)
- [Rendering](core-systems-rendering)
- [Shape / Object Model](core-systems-shape)
- [Tool System](core-systems-tool)
- [UI System](core-systems-ui)
- [Input and Canvas Routing](core-systems-input)
- [Selection System](core-systems-selection)

## Guides

- [Data Flow](data-flow)
- [Extending the Project](extending)
- [C Contributor Guide (EN)](../c-language-must-know-for-gldraw.en.md)
- [C 贡献者指南 (ZH)](../c-language-must-know-for-gldraw.md)
- [Known Issues](known-issues)
- [FAQ](faq)
