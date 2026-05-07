# Architecture

GLDraw is a canvas-oriented drawing editor built around a small application
shell and a layered editor workspace. The current structure is intentionally
split so that document state, session state, UI composition, and platform glue
can evolve independently.

## Top-Level Runtime Shape

At startup, `main()` calls `app_run()`, which initializes the application shell,
creates the workspace, registers built-in manifests, wires GLFW callbacks, and
then enters the frame loop.

The central runtime container is `Workspace` in `include/app/workspace.h`:

```text
Workspace
  -> EditorCore
  -> EditorSession
  -> EditorServices
```

Each layer has a different responsibility:

- `EditorCore` owns backend editing state: `Document`, `CommandExecutor`,
  `CanvasView`, and `ToolController`.
- `EditorSession` owns runtime-only editor state: selection, clipboard, keymap,
  layout snapshot, dialog state, current path, dirty tracking, and status text.
- `EditorServices` exposes application-owned callbacks such as save, save as,
  load, export, and deferred top-level actions.

This boundary keeps persisted drawing data out of UI-only and application-only
logic.

## Application Layer

`src/app/application.c` is now mostly orchestration:

- platform/window bootstrap and shutdown
- GLFW callback registration
- per-frame main loop sequencing
- subsystem assembly

Supporting application modules handle narrower responsibilities:

- `application_file_actions.c` coordinates open/save/save as/export flows
- `application_dialog_actions.c` coordinates dialog-facing status and
  confirmation flows
- `application_runtime.c` owns cursor, viewport, canvas-boundary, and
  `ToolEvent` helper logic
- `workspace_service.c` owns workspace-level document lifecycle operations
- `registration_manifest.c` owns unified boot-time registration sequencing

## Registration Model

Built-in extensions are registered through app-level manifests rather than being
hard-coded into runtime modules.

```text
workspace_init()
  -> app_register_all_manifests()
      -> object_manifest_register_all()
      -> tool_manifest_register_all()
```

Relevant files:

- `src/app/extension_manifest.c`
- `src/app/tool_manifest.c`
- `src/app/registration_manifest.c`
- `src/app/manifest_runner.c`

`include/app/extension_loader.h` remains as a compatibility shim for the older
object-extension registration names.

## Frame Loop

The frame loop in `app_run()` is intentionally stable:

```text
poll events
-> build editor view model
-> build UI for this frame
-> sync canvas viewport
-> draw render scene
-> flush pending export if requested
-> render UI
-> swap buffers
```

That sequencing keeps the UI and renderer reading a coherent snapshot of the
workspace for each frame.

## Input and Action Flow

Platform callbacks land in `application.c`, which performs lightweight
coordination before forwarding work:

- pointer events are filtered through UI hit testing
- `application_runtime.c` prepares `ToolContext` and `ToolEvent`
- tool pointer events go through `ToolController`
- keyboard shortcuts go through `input_router`
- UI actions emit `EditorAction` values into `command_dispatcher`

The result is that application, UI, and tool code do not all mutate document
state directly through ad hoc paths.

## Persistence and Undo

Persistence and editing history are separate concerns:

- `document_save_json()` and `document_load_json()` handle JSON persistence
- `workspace_service.c` resets runtime-only state after loads
- `CommandExecutor` owns undo/redo, transactions, merge behavior, and memory
  budget trimming

This keeps durable document content separate from transient runtime state such
as active selection, clipboard contents, tool instances, and dialog state.

## UI Boundary

The UI is no longer a single monolithic implementation file. Its current split
is roughly:

- `ui_runtime.c` for lifecycle, frame build, action bridge, and layout access
- `ui_theme_runtime.c` for theme loading and hot reload
- `ui_layout.c` for layout publication helpers
- `ui_dialog_runtime.c` for dialog runtime helpers
- `ui_chrome.c` for top-level chrome composition
- `ui_inspector_panel.c` for inspector rendering

The UI reads `EditorViewModel` and emits actions. Business rules stay in the
workspace, command, and application layers.

## Design Intent

The current architecture aims for:

- stable object identity and document persistence semantics
- command-based undo/redo for all durable edits
- explicit seams between application shell, editing core, and UI
- manifest-driven registration for object and tool extension points
- incremental refactors that can be built, tested, and reverted independently
