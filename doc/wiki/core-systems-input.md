# Input Handling

## Overview

Input handling bridges GLFW events and the tool system, converting coordinates and routing events to the current tool.

## Coordinate Conversion

GLFW uses window coordinates with origin at the **top-left**. OpenGL uses normalized device coordinates with origin at the **bottom-left**.

**Conversion Formula**:

OpenGL NDC range: [-1, 1] for both axes. Center of screen: (0, 0).

```c
x_ndc = (window_x / width) * 2.0 - 1.0;
y_ndc = 1.0 - (window_y / height) * 2.0;
```

## Mouse Button Events

```c
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
```

Routes to:
- `on_down`: When button pressed (action == GLFW_PRESS)
- `on_up`: When button released (action == GLFW_RELEASE)

The `shift_held` parameter is passed to tools for multi-selection support.

## Mouse Motion Events

```c
cursor_pos_callback(GLFWwindow* window, double x, double y)
```

Routes to current tool's `on_move` handler with converted coordinates.

## Keyboard Events

```c
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
```

| Key | Action |
|-----|--------|
| ESC | Exit application |
| Ctrl+Z | Delete last shape |
| 1 | Switch to LINE draw tool |
| 2 | Switch to CIRCLE draw tool |
| 3 | Switch to RECT draw tool |
| S | Switch to SELECT tool |

## Nuklear Mouse Blocking

Before routing mouse events to the current tool, `input.c` checks:
```c
if (nuklear_ui_blocks_mouse_input()) {
    return;  // Don't route to tool
}
```

This prevents accidental shape creation when clicking on the property panel.

## Key Files

- `include/core/input.h` - Public API
- `include/core/nuklear_ui.h` - `nuklear_ui_blocks_mouse_input()`
- `src/input.c` - Implementation
