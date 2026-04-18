# Data Flow

## Main Loop

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

```
GLFW callback (application.c)
    -> ui_system_blocks_pointer() / ui_system_point_in_canvas()
    -> ToolEvent { screen_pos, world_pos, delta_screen, delta_world, button, mods }
    -> tool_controller_pointer_down() / tool_controller_pointer_move() / tool_controller_pointer_up()
    -> ToolVTable callbacks (tool-specific)
    -> Document mutation via document_*() or CanvasView mutation via canvas_view_*
```

## Drawing Flow (Line / Rectangle / Ellipse)

```
pointer_down (ShapeTool)
    -> state->anchor = world_pos
    -> state->current = world_pos
    -> tool->overlay_object = object_create_line/rect/ellipse()  [translucent preview]

pointer_move (ShapeTool)
    -> state->current = world_pos
    -> object_destroy(tool->overlay_object)
    -> tool->overlay_object = object_create_line/rect/ellipse()  [rebuild preview]

pointer_up (ShapeTool)
    -> destroy_overlay(tool)  [free translucent preview]
    -> build final object: build_shape_object(kind, anchor, current)
    -> add_object(document, object)  [assigns ObjectId via next_id]
    -> document_touch()
    -> tool_commit_document_change()  [push HISTORY_ENTRY_SNAPSHOT]
```

## Selection Flow

```
pointer_down (SelectTool)
    -> canvas_view_pick_object()
    -> if hit: document_selection_add/remove/toggle()
    -> tool_commit_document_change()  [push HISTORY_ENTRY_SNAPSHOT]
    -> state->dragging = 1, state->drag_object_ids = selection

pointer_move (SelectTool, while dragging)
    -> for each drag_object_id: object_translate(object, delta_world)
    -> state->drag_delta_total += delta_world
    -> NO history push (avoids flooding undo stack)

pointer_up (SelectTool, after drag)
    -> document_history_push_translate_edit()
    -> tool_commit_document_change()
```

## Render Flow

```
render_system_draw()
    -> glClear()  [canvas background]
    -> glEnable(GL_SCISSOR_TEST)
    -> glScissor(viewport)
    -> render_grid()  [world_to_screen transform per line]
    -> for each document object (oldest to newest):
        -> object_get_bounds()
        -> if visible: object_write_path_points()
        -> world_to_screen per vertex
        -> batch append to VBO
    -> glBufferData()  [batched GPU upload]
    -> glDrawArrays()  [batched line draw]
    -> render overlay_object if present (ShapeTool preview)
    -> glDisable(GL_SCISSOR_TEST)
```

## Property Editing Flow (Inspector)

```
Inspector widget change (ui_system.c)
    -> ui_commit_scalar_edit()
        -> document_history_push_scalar_edit(object_id, key, before, after)
        -> object_set_scalar(object, key, after)
        -> document_touch()
```

## Persistence Flow

```
Ctrl+S / Menu Save
    -> application.c: app_on_save()
    -> document_save_json(path ?: "document.json")

Ctrl+O / Menu Load
    -> application.c: app_on_open()
    -> document_load_json(path)
    -> document_history_reset()
    -> tool_controller_reset_active_tool()
    -> CanvasView state preserved (zoom, pan, viewport)
```

## Theme Hot-Reload Flow

```
ui_system_build()
    -> ui_system_poll_theme_hot_reload()
    -> if themes/ changed: ui_theme_reload_external()
    -> on parse error: keep previous theme + status bar error message
```
