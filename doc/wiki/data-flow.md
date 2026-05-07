# Data Flow

This page describes the current high-level runtime flow after the application,
workspace, tool, document, and UI splits. Function names are included where
they help orientation, but the emphasis here is module boundaries rather than
every internal helper.

## Startup and Registration

```
main()
    -> app_run()
    -> app_init()
    -> workspace_init()
    -> app_register_all_manifests()
        -> object_manifest_register_all()
        -> tool_manifest_register_all()
    -> tool_controller_init()
    -> ui_system_create()
    -> render_system_create()
```

Notes:

- Built-in object types and tools are now registered through app-level
  manifests.
- `include/app/extension_loader.h` remains as a compatibility shim for the
  older object-extension naming.

## Main Loop

```c
while (!platform_window_should_close(&app->window)) {
    platform_window_poll_events();
    ui_system_begin_frame(app->ui);
    editor_viewmodel_build(&app->view_model, &app->workspace);
    ui_system_build(app->ui, &app->view_model);
    application_runtime_update_canvas_viewport(app);
    render_system_draw(...);
    application_flush_pending_export_png(app);
    ui_system_render(app->ui);
    platform_window_swap_buffers(&app->window);
}
```

Notes:

- `application.c` stays focused on orchestration.
- `EditorViewModel` is built before UI composition so the UI reads a stable
  frame snapshot instead of reaching into editing subsystems ad hoc.
- Pending PNG export is flushed after the main scene draw, using the same render
  path as the interactive frame.

## Pointer and Key Input Flow

```
GLFW callback (application.c)
    -> application_runtime_sync_*()
    -> ui_system_handle_*() / ui_system_blocks_pointer()
    -> application_runtime_make_tool_event()
    -> tool_controller_pointer_*() or input_router_handle_key()
    -> active ToolDescriptor callbacks
    -> workspace actions / command execution / canvas updates
```

Notes:

- `application_runtime.c` now owns the small coordination helpers that keep
  cursor state, canvas bounds, viewport, and `ToolEvent` construction in sync.
- Pointer events are filtered through UI hit-testing before the active tool sees
  them, unless the tool has already captured the pointer.
- Keyboard shortcuts are routed through `input_router` so global editor actions
  win over tool-local key handling when appropriate.

## UI Action Flow

```
ui_system_build()
    -> menu / rail / inspector / dialog widgets
    -> ui_system_emit_action()
    -> command_dispatcher_action_callback()
    -> workspace actions or command registry execution
```

Notes:

- UI modules now focus on presentation and action emission.
- Business rules such as save/load/export, dialog confirmation, and command
  dispatch live outside the UI widgets themselves.

## Tool Editing Flow

```
tool pointer callback
    -> update tool-local state
    -> optional overlay object preview
    -> commit through command executor for durable document mutations
```

Typical examples:

- Shape tools maintain a temporary overlay during drag, then commit a create
  command on release.
- Selection dragging uses workspace preview state during the drag and commits a
  move command when the gesture completes.

This keeps undo/redo behavior stable while avoiding command-stack spam during
continuous pointer motion.

## Persistence Flow

```
Menu / shortcut / dialog action
    -> workspace service callback
    -> application_file_actions.c
    -> workspace_service_save_*() / workspace_service_load_*()
    -> document_save_json() / document_load_json()
    -> runtime reset of command/tool/selection state as needed
```

Notes:

- File picker handling, Save As flow, startup load, and PNG export requests are
  coordinated by the application-level file action helpers.
- Core document load/save remains in the persistence layer.
- Workspace services reset runtime-only state after loads so command history,
  tool instances, clipboard state, and selection stay coherent.

## Render Flow

```
render_system_draw()
    -> consume document + selection + canvas state
    -> query visible objects
    -> build draw data
    -> draw document geometry
    -> draw selection preview / active tool overlay
```

Notes:

- Rendering reads model state but does not own editing rules.
- Document spatial queries stay inside the document subsystem; the renderer only
  asks for the visible result set.

## Theme and Dialog Runtime

```
ui_system_build()
    -> ui_system_poll_theme_hot_reload()
    -> ui_modal_dialogs()
    -> layout snapshot publish
```

Notes:

- Theme reload, dialog runtime, chrome composition, inspector layout, and layout
  publishing are now split across dedicated UI source files.
- The workspace stores the resulting layout snapshot so application/runtime code
  can keep canvas bounds and input routing aligned with the current frame.
