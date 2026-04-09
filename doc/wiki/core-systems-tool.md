# Tool System

## Overview

The tool system handles all mouse and keyboard input using a polymorphic vtable pattern. Each tool implements mouse event handlers that operate on the ShapeManager and SelectionManager.

## ToolVTable

```c
typedef struct {
    const char* (*name)(const Tool*);
    void (*on_down)(Tool*, float x, float y, SelectionManager* sel, int shift_held);
    void (*on_move)(Tool*, float x, float y, SelectionManager* sel);
    void (*on_up)(Tool*, SelectionManager* sel);
} ToolVTable;
```

## Tool Structure

```c
struct Tool {
    ToolVTable* vtable;     // Function pointers
    void* ctx;              // Tool-specific context
};
```

## Built-in Tools

### DrawTool

Creates shapes via mouse drag.

**Context Data**: `DrawToolCtx`
```c
typedef struct {
    float p1[2];           // Start position
    float p2[2];           // Current/end position
    Shape* preview;        // Live preview shape
    ShapeType shape_type;  // LINE, CIRCLE, or RECT
} DrawToolCtx;
```

**Behavior**:
- `on_down`: Store start position, create preview shape
- `on_move`: Update preview shape geometry
- `on_up`: Finalize shape, add to ShapeManager, destroy preview

### SelectTool

Selects and moves shapes.

**Context Data**: `SelectToolCtx`
```c
typedef struct {
    bool dragging;         // Currently dragging
    float drag_start[2];    // Mouse position when drag started
    float shape_start[2];  // Shape position when drag started
} SelectToolCtx;
```

**Behavior**:
- `on_down`: Hit-test shapes, add to selection
- `on_move`: Translate all selected shapes
- `on_up`: Stop dragging

## ToolManager

Global tool management:

```c
void toolmanager_init(void);
void toolmanager_set_tool(Tool* tool);
Tool* toolmanager_get_current(void);
```

## Key Files

- `include/core/tool.h` - Tool struct and vtable
- `include/core/tool_manager.h` - ToolManager API
- `include/core/draw_tool.h` - DrawTool API
- `include/core/select_tool.h` - SelectTool API
- `src/tool.c` - Tool base implementation
- `src/tool_manager.c` - ToolManager implementation
- `src/draw_tool.c` - DrawTool implementation
- `src/select_tool.c` - SelectTool implementation

## Input Routing

```
GLFW mouse_button_callback
         ↓
input.c: mouse_button_callback
         ↓
ToolManager → current_tool → on_down/on_up
```
