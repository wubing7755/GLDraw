# Core Systems

> Audience: contributors, maintainers
> Purpose: define subsystem ownership and common boundaries
> Source of truth: source tree under `include/` and `src/`
> Last reviewed with code: 2026-05-07
> Related: [overview.md](overview.md), [../reference/file-map.md](../reference/file-map.md)

## At a Glance

| Subsystem | Owns | Does not own | First file to read |
|---|---|---|---|
| Document | objects, layers, IDs, revision, spatial queries | selection, clipboard, undo/redo | `include/document/document.h` |
| Object system | object descriptors, runtime object creation, property schema | command history, tool input | `include/document/object.h` |
| Commands | durable mutations, undo/redo, merge, transactions | UI composition, rendering | `include/commands/command.h` |
| Canvas | viewport, zoom, pan, transforms | persistence semantics | `include/canvas/canvas_view.h` |
| Tools | active-tool runtime, input dispatch, overlays | final render ownership | `include/tools/tool.h` |
| Workspace | core/session/services aggregation | platform windowing | `include/app/workspace.h` |
| Application | bootstrap, frame loop, top-level routing | document truth | `src/app/application.c` |
| UI | menus, inspector, dialogs, action emission | durable edit semantics | `src/ui/ui_runtime.c` |
| Render | draw translation and submission | editor business rules | `src/render/render_system.c` |

## Document

Primary files:

- `include/document/document.h`
- `src/document/document.c`
- `src/document/document_objects.c`
- `src/document/document_layers.c`
- `src/document/document_spatial_index.c`

Responsibilities:

- own `GraphicObject` storage
- own layer list and active layer state
- preserve stable object and layer IDs
- track document revision
- answer visible-area and point spatial queries

Non-responsibilities:

- selection state
- clipboard state
- undo/redo history
- dialog state

## Object System

Primary files:

- `include/document/object.h`
- `src/document/object_registry.c`
- `src/document/object_runtime.c`
- `src/document/object_properties.c`

Responsibilities:

- define `GraphicObjectDescriptor`
- register and look up object types
- create, clone, destroy, and query runtime objects
- expose property schema and scalar property helpers
- support serialization through descriptor callbacks

Extension references:

- `src/document/object_line.c`
- `src/document/object_rect.c`
- `src/document/object_ellipse.c`
- `src/document/object_fake_star.c`

## Commands

Primary files:

- `include/commands/command.h`
- `src/commands/command.c`
- `src/commands/command_executor.c`
- `src/commands/command_object_ops.c`
- `src/commands/command_layer_ops.c`
- `src/commands/command_transaction.c`

Responsibilities:

- execute document mutations through command objects
- own undo/redo history and cursor state
- support merge semantics
- support transactions with rollback on failure
- enforce history memory budget trimming

Design boundary:

- durable edits should flow through commands
- command implementations may mutate `Document`
- callers outside the command layer should avoid ad hoc durable mutation

## Canvas

Primary files:

- `include/canvas/canvas_view.h`
- `src/canvas/canvas_view.c`

Responsibilities:

- own viewport state
- own zoom and pan state
- convert world coordinates to screen coordinates and back
- provide view-aware picking helpers

Design boundary:

- canvas code provides spatial context
- canvas code does not define persistence or undo semantics

## Tools

Primary files:

- `include/tools/tool.h`
- `include/tools/tool_controller.h`
- `src/tools/tool_registry.c`
- `src/tools/tool_runtime.c`
- `src/tools/tool_input_dispatch.c`

Built-in tool references:

- `src/tools/tool_select.c`
- `src/tools/tool_pan.c`
- `src/tools/tool_shape.c`

Responsibilities:

- store tool descriptors in the registry
- own active-tool runtime instances
- dispatch pointer, key, and wheel input
- expose temporary overlay objects for preview rendering

Design boundary:

- tools may manage preview state
- final durable edits should typically end in command execution

## Workspace

Primary files:

- `include/app/workspace.h`
- `src/app/workspace_service.c`
- `src/app/workspace_actions.c`
- `src/app/workspace_dialogs.c`

Responsibilities:

- aggregate runtime state into core, session, and services layers
- hold selection, clipboard, layout snapshot, and dialog state
- expose application integration callbacks
- coordinate top-level workspace actions and status messaging

## Application

Primary files:

- `src/app/application.c`
- `src/app/application_runtime.c`
- `src/app/application_file_actions.c`
- `src/app/application_dialog_actions.c`
- `src/app/command_dispatcher.c`
- `src/app/command_registry.c`

Responsibilities:

- bootstrap and shut down the process-local runtime
- register GLFW callbacks
- run the frame loop
- route platform events into editor flows
- coordinate file dialogs and export requests

Design boundary:

- application code orchestrates
- it should not become a second document or command subsystem

## UI

Primary files:

- `include/ui/ui_system.h`
- `src/ui/ui_runtime.c`
- `src/ui/ui_theme_runtime.c`
- `src/ui/ui_layout.c`
- `src/ui/ui_dialog_runtime.c`
- `src/ui/ui_chrome.c`
- `src/ui/ui_inspector_panel.c`

Responsibilities:

- build menus, toolbar/rail, inspector, status area, and dialogs
- publish layout bounds back to the workspace
- emit actions rather than embedding all business rules in widgets

## Render

Primary files:

- `include/render/render_system.h`
- `src/render/render_system.c`
- `src/render/`

Responsibilities:

- translate document and canvas state into draw calls
- render document geometry, overlays, and selection previews
- keep graphics backend details downstream from editor rules

## Optional Scripting Hook

Primary files:

- `src/script/script_runtime_lua.c`
- `src/tools/script_tool_lua.c`

This is an optional extension area rather than the core editing model. If scripting is enabled, tool registration still flows through the app-level manifest path.
