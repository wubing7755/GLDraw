# File Map

> Audience: contributors, maintainers
> Purpose: provide fast entrypoints into the source tree by task
> Source of truth: current source tree layout
> Last reviewed with code: 2026-05-26
> Related: [../architecture/overview.md](../architecture/overview.md)

## First Entry Points

- `src/main.c`
  Process entrypoint, forwards to `app_run()`.
- `src/app/application.c`
  App lifecycle, subsystem setup/teardown, and frame loop.
- `src/app/application_callbacks.c`
  Platform window callbacks for pointer, key, scroll, framebuffer, and close events.
- `src/app/application_workspace_services.c`
  Application-owned workspace save/load/export and action executor callbacks.
- `include/app/editor_controller.h`
  Workspace-level editor runtime facade for tool input and render scene state.
- `include/app/workspace.h`
  Main runtime container and workspace-facing helpers.
- `include/commands/command.h`
  Command and executor public interface.
- `include/document/object.h`
  Object descriptor and registry API.
- `include/tools/tool.h`
  Tool descriptor and registry API.

## If You Want To...

### Understand boot-time registration

- `src/app/registration_manifest.c`
- `src/app/extension_manifest.c`
- `src/app/tool_manifest.c`
- `src/app/manifest_runner.c`

### Understand document storage and queries

- `src/document/document.c`
- `src/document/document_objects.c`
- `src/document/document_layers.c`
- `src/document/document_spatial_index.c`

### Understand durable edit behavior

- `include/app/command_availability.h`
- `src/app/command_availability.c`
- `include/app/command_catalog.h`
- `src/app/command_catalog.c`
- `include/app/command_types.h`
- `include/app/ui_dialog_types.h`
- `include/app/workspace_layout_types.h`
- `include/app/workspace_service_types.h`
- `src/commands/command_executor.c`
- `src/commands/command_create_object.c`
- `src/commands/command_delete_selection.c`
- `src/commands/command_move_objects.c`
- `src/commands/command_object_checks.c`
- `src/commands/command_object_ops.c`
- `src/commands/command_paste_objects.c`
- `src/commands/command_set_property.c`
- `src/commands/command_layer_ops.c`
- `src/commands/command_transaction.c`
- `src/app/command_dispatcher.c`
- `src/app/editor_action_handler.c`
- `src/app/editor_controller.c`
- `src/app/command_registry.c`
- `src/app/workspace_clipboard.c`
- `include/app/workspace_dialog_commands.h`
- `src/app/workspace_dialog_commands.c`
- `include/app/workspace_edit_commands.h`
- `src/app/workspace_edit_commands.c`
- `include/app/workspace_file_commands.h`
- `src/app/workspace_file_commands.c`
- `include/app/workspace_tool_commands.h`
- `src/app/workspace_tool_commands.c`
- `src/app/workspace_view_commands.c`

### Understand tool interactions

- `src/tools/tool_runtime.c`
- `src/tools/tool_input_dispatch.c`
- `src/tools/tool_select.c`
- `src/tools/tool_pan.c`
- `src/tools/tool_shape.c`

### Understand coordinate and viewport behavior

- `src/canvas/canvas_view.c`
- `include/canvas/canvas_view.h`

### Understand UI composition

- `src/ui/ui_runtime.c`
- `src/ui/ui_frame.c`
- `src/ui/ui_chrome.c`
- `src/ui/ui_context_menu.c`
- `src/ui/ui_context_menu_render.c`
- `src/ui/ui_inspector_panel.c`
- `src/ui/ui_layer_panel.c`
- `src/ui/ui_dialog_runtime.c`
- `src/ui/ui_layout.c`
- `src/ui/ui_theme_apply.c`
  Nuklear stylesheet application for theme tokens.
- `src/ui/ui_theme.c`
- `src/ui/ui_theme_builtin.c`
  Built-in theme descriptors and token presets.
- `src/ui/ui_theme_external.c`
  External theme registry, file/directory reload, reload errors, and directory signatures.
- `src/ui/ui_theme_parse.c`
  Theme JSON value extraction, color parsing, path-derived IDs, override application, and token clamping.
- `src/ui/ui_theme_settings.c`
  Selected theme ID settings load/save.
- `src/ui/ui_theme_internal.h`
  Private theme-system constants and cross-module declarations.

### Understand rendering

- `src/render/render_system.c`
- `src/render/`

### Understand persistence

- `src/document/persistence.c`
- `src/document/persistence_json.c`
- `src/document/persistence_json.h`
- `src/document/persistence_layers.c`
- `src/document/persistence_layers.h`
- `src/document/persistence_objects.c`
- `src/document/persistence_objects.h`
- `src/document/persistence_write.c`
- `src/app/workspace_service.c`
- `src/app/application_file_actions.c`

### Understand shortcuts and menu wiring

- `src/input/keymap.c`
- `src/ui/ui_menu_def.c`
- `src/ui/ui_menubar.c`
- `src/ui/ui_menubar_render.c`
  Menu bar and context menu rendering are split from their lifecycle/state modules.

### Understand test coverage

- `tests/test_commands.c`
- `tests/test_document_core.c`
- `tests/test_document.c`
- `tests/test_registry.c`
- `tests/test_ui_logic.c`
- `tests/test_ui_theme.c`
- `tests/test_workspace_service.c`
