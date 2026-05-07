# Core Systems Overview

This page is a quick map of the main runtime subsystems and the boundaries they
own today.

## Document

Location:

- `include/document/document.h`
- `src/document/document.c`
- `src/document/document_objects.c`
- `src/document/document_layers.c`
- `src/document/document_spatial_index.c`

Responsibilities:

- own `GraphicObject` storage
- own layer list and active layer state
- assign and preserve stable object and layer IDs
- track document revision
- answer visible-area and point spatial queries

Non-responsibilities:

- selection state
- clipboard state
- dialog state
- undo/redo history

## Object System

Location:

- `include/document/object.h`
- `src/document/object_registry.c`
- `src/document/object_properties.c`
- `src/document/object_runtime.c`

Responsibilities:

- register and look up `GraphicObjectDescriptor` values
- create, clone, destroy, and query runtime objects
- expose scalar property and schema helpers
- support serialization through descriptor callbacks

Key pattern:

- new object types register via `register_object_type()`
- object manifests assemble built-ins at app startup

## Commands

Location:

- `include/commands/command.h`
- `src/commands/command_executor.c`
- `src/commands/command_object_ops.c`
- `src/commands/command_layer_ops.c`
- `src/commands/command_transaction.c`

Responsibilities:

- execute document mutations through command objects
- own undo/redo logs and cursor state
- support command merge semantics
- support transactions with rollback on failure
- enforce history memory budget trimming

External contract:

- public command creation APIs remain in `include/commands/command.h`
- callers should treat command execution as the durable edit path

## Canvas

Location:

- `include/canvas/canvas_view.h`
- `src/canvas/`

Responsibilities:

- own viewport and zoom/pan state
- convert world coordinates to screen coordinates and back
- provide view-aware picking helpers

The canvas does not own document edits. It provides the spatial context that
tools, rendering, and picking use.

## Tools

Location:

- `include/tools/tool.h`
- `include/tools/tool_controller.h`
- `src/tools/tool_registry.c`
- `src/tools/tool_runtime.c`
- `src/tools/tool_input_dispatch.c`

Responsibilities:

- store tool descriptors in the registry
- create and destroy tool runtime instances
- track the active tool and pointer anchor
- dispatch pointer, key, and scroll input to the active tool
- expose transient overlay objects for preview rendering

Key pattern:

- tool descriptors define behavior through callbacks
- built-in tools are registered through `src/app/tool_manifest.c`

## Workspace

Location:

- `include/app/workspace.h`
- `src/app/workspace_service.c`
- `src/app/workspace_actions.c`
- `src/app/workspace_dialogs.c`

Responsibilities:

- aggregate editing state into `EditorCore`, `EditorSession`, and
  `EditorServices`
- store selection, clipboard, layout snapshot, and dialog state
- expose application integration callbacks
- coordinate top-level workspace actions and status messaging

This is the main seam between the app shell and the editor backend.

## Application

Location:

- `src/app/application.c`
- `src/app/application_file_actions.c`
- `src/app/application_dialog_actions.c`
- `src/app/application_runtime.c`

Responsibilities:

- bootstrap and shutdown the process-local runtime
- register GLFW callbacks
- run the frame loop
- translate platform events into editor/runtime calls
- coordinate file dialogs and export requests

The application layer should not re-implement document or command logic.

## UI

Location:

- `include/ui/ui_system.h`
- `src/ui/ui_runtime.c`
- `src/ui/ui_theme_runtime.c`
- `src/ui/ui_layout.c`
- `src/ui/ui_dialog_runtime.c`
- `src/ui/ui_chrome.c`
- `src/ui/ui_inspector_panel.c`

Responsibilities:

- build the visible editor chrome
- render menus, rail, inspector, status bar, and dialogs
- publish layout bounds back to the workspace
- emit `EditorAction` values rather than directly encoding business rules

The UI reads `EditorViewModel` as its primary frame input.

## Render

Location:

- `include/render/render_system.h`
- `src/render/render_system.c`
- `src/render/`

Responsibilities:

- translate document and canvas state into draw calls
- render main geometry, overlays, and selection previews
- keep rendering backend concerns separate from editing rules

The renderer is downstream from document, canvas, and tool state. It should not
be the source of truth for editor behavior.

## How They Fit Together

At a high level:

```text
application
-> workspace
-> tools / commands / document / canvas
-> render + ui
```

More concretely:

- application orchestrates the frame and event loop
- workspace holds the shared runtime state
- tools turn input into command execution or canvas changes
- commands mutate document state with undo/redo support
- UI reads view-model state and emits actions
- renderer draws the current document and tool/session overlays
