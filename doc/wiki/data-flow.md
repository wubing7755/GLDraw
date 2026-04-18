# Data Flow

## Main Loop

The current frame loop is:

```c
while (!platform_window_should_close(&app.window)) {
    platform_window_poll_events();
    ui_system_begin_frame(app.ui);
    ui_system_build(app.ui, &app.workspace);
    update_canvas_viewport(app);
    render_system_draw(app.renderer, &app.workspace.document, &app.workspace.canvas, overlay);
    ui_system_render(app.ui);
    platform_window_swap_buffers(&app.window);
}
```

## Event Flow

```text
GLFW
    -> application.c callback
    -> ui_system_blocks_pointer / ui_system_point_in_canvas
    -> ToolEvent { screen_pos, world_pos, delta_* }
    -> tool_controller_*
    -> document or canvas update
```

## Drawing Flow

```text
Pointer down
    -> shape tool stores anchor point
Pointer move
    -> shape tool rebuilds overlay object
Pointer up
    -> shape tool creates a document object
    -> document assigns ObjectId
```

## Selection Flow

```text
Pointer down in Select tool
    -> canvas picking
    -> selection change (snapshot history path)
Pointer move while dragging
    -> translate fixed object-id set in world space (no per-move history push)
Pointer up after drag
    -> one translate transaction entry pushed to history
```

## Render Flow

```text
Document object
    -> object_write_path_points()
    -> world polyline
    -> canvas world_to_screen
    -> frame batch append
    -> batched GPU upload/flush
    -> OpenGL line draw
```

## Property Editing Flow

```text
Inspector widget change
    -> object id/key/before/after capture
    -> scalar transaction push to history
    -> next frame draws updated geometry
```

## Persistence Flow

```text
Ctrl+S / Toolbar Save
    -> application save command
    -> document_save_json()
    -> current path or document.json

Ctrl+O / Toolbar Load
    -> application load command
    -> document_load_json()
    -> history reset
    -> tool transient state reset
    -> canvas view preserved

Theme reload flow
    -> ui_system_poll_theme_hot_reload()
    -> ui_theme_reload_external()
    -> on parse failure: keep previous runtime theme set + status error summary
```
