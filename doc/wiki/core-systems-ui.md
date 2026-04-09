# UI System

## Overview

The UI system uses **Nuklear**, a header-only immediate-mode GUI library, to provide a property panel for editing selected shapes.

## Property Panel

The property panel displays:
- **Shape type** (read-only)
- **Color sliders** (R, G, B, A) - applies to all selected shapes
- **Line width slider** - applies to all selected shapes
- **Selection count**

## Nuklear Integration

### Initialization

```c
void init_nuklear_ui(GLFWwindow* window);
```

Creates a Nuklear context and configures it for OpenGL 3.3 with GLFW.

### Frame Lifecycle

```c
nuklear_new_frame();     // Called before UI building
nuklear_build_ui(SelectionManager* sel);  // Build UI widgets
nuklear_render(void);   // Render UI to OpenGL
```

### Mouse Input Blocking

```c
bool nuklear_ui_blocks_mouse_input(void);
```

Returns `true` if the mouse cursor is over any Nuklear UI widget. Used by `input.c` to prevent tool activation when interacting with the UI.

## UI Layout

```
┌─────────────────────────────┐
│ Properties                  │
├─────────────────────────────┤
│ Type: LINE                  │
│                             │
│ Color                       │
│ R ──────●────────── 1.00    │
│ G ──────●────────── 1.00    │
│ B ──────●────────── 1.00    │
│ A ──────●────────── 1.00    │
│                             │
│ Line Width                  │
│ ──────●──────────── 1.00    │
│                             │
│ Selected: 1 shape(s)       │
└─────────────────────────────┘
```

Position: `x=580, y=50`, `width=220, height=400`

## Key Files

- `include/core/nuklear_ui.h` - Public API
- `src/nuklear_ui.c` - Implementation
- `include/nuklear/` - Nuklear library headers

## Dependencies

Nuklear is configured with the following options:
- `NK_INCLUDE_FIXED_TYPES` - Fixed point types
- `NK_INCLUDE_STANDARD_IO` - Standard I/O
- `NK_INCLUDE_DEFAULT_ALLOCATOR` - Default memory allocator
- `NK_INCLUDE_VERTEX_BUFFER_OUTPUT` - Vertex buffer output
- `NK_INCLUDE_COMMAND_USERDATA` - Command userdata
- `NK_GLFW_GL3_IMPLEMENTATION` - GLFW+OpenGL3 implementation
