# Architecture Overview

## High-Level Design

GLDraw follows a **modular, layered architecture** with clear separation between subsystems:

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  main.c - Entry point, initialization, main loop            │
├─────────────────────────────────────────────────────────────┤
│                     Tool System Layer                        │
│  ToolManager → DrawTool / SelectTool (vtable pattern)       │
├─────────────────────────────────────────────────────────────┤
│                     Shape System Layer                       │
│  ShapeManager → ShapeRegistry → Shapes (vtable pattern)      │
├─────────────────────────────────────────────────────────────┤
│                     Render System Layer                      │
│  Renderer → VAO/VBO → OpenGL 3.3                            │
├─────────────────────────────────────────────────────────────┤
│                     Platform Layer                           │
│  GLFW (window/input) + Nuklear (GUI)                        │
└─────────────────────────────────────────────────────────────┘
```

## Core Modules

### 1. Window System (`window.c`)
- Creates GLFW window with OpenGL 3.3 Core Profile
- Handles framebuffer resize callback
- Manages event polling and buffer swapping

### 2. Renderer (`renderer.c`)
- Manages VAO/VBO for shape geometry
- Tracks dirty state to optimize GPU uploads
- Renders shapes with per-vertex RGBA color
- Uses GL_LINES/GL_LINE_LOOP for shape outlines

### 3. Shader System (`shader.c`)
- Loads GLSL 330 core shaders from files
- Simple passthrough vertex/fragment shaders
- Single shader program for all shapes

### 4. Shape System (`shape.c`, `shape_registry.c`)
- Polymorphic shape types via vtable
- Registry pattern for extensibility
- Shape types: LINE, CIRCLE, RECT

### 5. ShapeManager (`shape_manager.c`)
- Dynamic array of Shape pointers (max 256)
- Revision tracking for cache invalidation

### 6. Tool System (`tool_manager.c`, `draw_tool.c`, `select_tool.c`)
- Polymorphic tools via vtable
- DrawTool: creates shapes via mouse drag
- SelectTool: selects and moves shapes

### 7. SelectionManager (`selection_manager.c`)
- Tracks currently selected shapes
- Supports multi-selection (max 64)

### 8. Input System (`input.c`)
- GLFW callbacks delegate to current tool
- Coordinate conversion (window → OpenGL space)
- Keyboard shortcuts for tool switching

### 9. Nuklear UI (`nuklear_ui.c`)
- Property panel for selected shape editing
- Color sliders (RGBA) and line width control

## Design Patterns

| Pattern | Usage |
|---------|-------|
| **VTable** | Shape and Tool polymorphism |
| **Registry** | Shape type registration without modification |
| **Singleton** | AppState, ToolManager global state |
| **Dirty Tracking** | Renderer avoids unnecessary GPU uploads |
| **Callback** | GLFW events delegate to current tool |

## Key Architectural Decisions

1. **VTable over enum switch** - Shapes and Tools use function pointers for extensibility
2. **Registry over factory** - New shapes register themselves at startup
3. **Tool-centric input** - All mouse/keyboard input routes through current tool
4. **Separate render from data** - Renderer only draws what ShapeManager provides
