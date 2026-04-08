# Project Structure

## Directory Layout

```
GLDraw/
├── CMakeLists.txt           # CMake build configuration
├── LICENSE.txt              # MIT License
├── README.md                # English documentation
├── include/                 # Public header files
│   ├── core/                # Project headers
│   │   ├── app_state.h
│   │   ├── draw_tool.h
│   │   ├── input.h
│   │   ├── macros.h         # Logging and utility macros
│   │   ├── nuklear_ui.h
│   │   ├── renderer.h
│   │   ├── select_tool.h
│   │   ├── selection_manager.h
│   │   ├── shape.h
│   │   ├── shape_impl.h
│   │   ├── shape_manager.h
│   │   ├── shape_registry.h
│   │   ├── shader.h
│   │   ├── tool.h
│   │   ├── tool_manager.h
│   │   └── window.h
│   ├── KHR/                 # GLAD platform headers
│   ├── glad/                # GLAD OpenGL loader
│   └── nuklear/             # Nuklear GUI (header-only)
├── shaders/                 # GLSL shaders
│   ├── basic.vert
│   └── basic.frag
├── src/                     # Source files (mirror of include/core)
│   ├── main.c               # Entry point
│   ├── app_state.c
│   ├── draw_tool.c
│   ├── input.c
│   ├── nuklear_ui.c
│   ├── renderer.c
│   ├── select_tool.c
│   ├── selection_manager.c
│   ├── shader.c
│   ├── shape.c
│   ├── shape_manager.c
│   ├── shape_registry.c
│   ├── tool.c
│   ├── tool_manager.c
│   ├── window.c
│   └── glad.c
├── doc/
│   ├── README-zh.md
│   └── wiki/                # This Wiki
└── build/                   # Build output
```

## Key Files and Responsibilities

### Entry Point

| File | Responsibility |
|------|----------------|
| `src/main.c` | Application entry point, initialization sequence, main render loop |

### Core Modules

| File | Header | Responsibility |
|------|--------|----------------|
| `window.c` | `window.h` | GLFW window creation, OpenGL context setup |
| `renderer.c` | `renderer.h` | VAO/VBO management, shape rendering |
| `shader.c` | `shader.h` | GLSL shader compilation and linking |
| `shape.c` | `shape.h`, `shape_impl.h` | Shape vtable, base shape operations |
| `shape_manager.c` | `shape_manager.h` | Dynamic array of shapes |
| `shape_registry.c` | `shape_registry.h` | Shape type registration |
| `tool.c` | `tool.h` | Tool vtable, base tool operations |
| `tool_manager.c` | `tool_manager.h` | Current tool management |
| `draw_tool.c` | `draw_tool.h` | Shape drawing tool implementation |
| `select_tool.c` | `select_tool.h` | Shape selection/movement tool |
| `selection_manager.c` | `selection_manager.h` | Selected shapes tracking |
| `input.c` | `input.h` | GLFW callbacks, coordinate conversion |
| `nuklear_ui.c` | `nuklear_ui.h` | Nuklear GUI, property panel |
| `app_state.c` | `app_state.h` | Global singleton state |

### Headers

| Header | Purpose |
|--------|---------|
| `core/shape.h` | Shape struct, ShapeVTable definition |
| `core/shape_impl.h` | Concrete shape implementations (LineImpl, CircleImpl, RectImpl) |
| `core/tool.h` | Tool struct, ToolVTable definition |
| `core/macros.h` | C11 logging macros (LOG_DEBUG, LOG_INFO, etc.) |

### Shaders

| File | Purpose |
|------|---------|
| `shaders/basic.vert` | Vertex shader - passes through position and color |
| `shaders/basic.frag` | Fragment shader - outputs solid color |
