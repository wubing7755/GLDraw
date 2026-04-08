# Data Flow

## Application Loop

The main loop follows a standard graphics application pattern:

```c
while (!window_should_close()) {
    poll_events();           // GLFW poll
    process_input();         // Route to current tool
    render_frame();          // OpenGL rendering
    nuklear_new_frame();     // Nuklear begin frame
    nuklear_build_ui();      // Build property panel
    nuklear_render();        // Render Nuklear UI
    swap_buffers();          // GLFW swap buffers
}
```

## Input Flow

```
GLFW Event (mouse/keyboard)
        ↓
input.c callbacks
        ↓
nuklear_ui_blocks_mouse_input() → true? → ignore event
        ↓ false
ToolManager → current_tool → on_down/on_move/on_up
        ↓
ShapeManager / SelectionManager modification
```

## Shape Creation Flow

```
DrawTool.on_down
        ↓
ShapeRegistry.create("LINE") / "CIRCLE" / "RECT"
        ↓
ShapeManager.add()
        ↓
renderer_mark_dirty()
```

## Render Flow

```
Main Loop render_frame()
        ↓
ShapeManager iteration
        ↓
For each shape:
    shape->vtable->write_geometry() → vertex buffer
    shape->vtable->get_vertex_count()
        ↓
glDrawArrays(primitive, 0, vertex_count)
        ↓
Selection highlight (yellow outline for selected)
        ↓
Nuklear UI render (on top)
```

## Data Update Flow

When a shape property changes (via Nuklear UI):

```
User adjusts slider
        ↓
nuklear_build_ui() → shape_set_color() / shape_set_line_width()
        ↓
shape->revision++
        ↓
renderer_mark_dirty()
        ↓
Next frame: renderer detects revision mismatch, re-uploads geometry
```

## Revision Tracking

The `revision` counter on each shape and `ShapeManager` enables efficient dirty checking:

```c
// renderer.c
if (sm->revision != renderer->cache_revision) {
    rebuild_vertex_buffer();  // Only when shapes changed
    renderer->cache_revision = sm->revision;
}
```
