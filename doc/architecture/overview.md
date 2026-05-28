# Architecture Overview

> Audience: contributors, maintainers
> Purpose: describe the current runtime structure and intended ownership boundaries
> Source of truth: `include/app/workspace.h`, `src/app/`, `include/commands/command.h`, `include/tools/tool.h`
> Last reviewed with code: 2026-05-07
> Related: [core-systems.md](core-systems.md), [data-flow.md](data-flow.md), [extension-model.md](extension-model.md), [refactor-roadmap.md](refactor-roadmap.md)

## Runtime Shape

GLDraw is a canvas-centered drawing editor built around a small application shell and a layered editor workspace.

The central runtime container is `Workspace`:

```text
Workspace
  -> EditorCore
  -> EditorSession
  -> EditorServices
```

Current ownership:

- `EditorCore`
  Owns backend editing state: `Document`, `CommandExecutor`, `CanvasView`, and `ToolController`.
- `EditorSession`
  Owns runtime-only state: selection, clipboard, layout snapshot, dirty tracking, current path, status message, and dialog state.
- `EditorServices`
  Owns application-supplied callbacks for save, save as, load, export, and deferred top-level actions.

## System-Level Rules

- Durable document edits should go through `CommandExecutor`.
- Objects and tools are descriptor-driven and registered through manifests.
- `CanvasView` owns spatial context, not document persistence semantics.
- UI should emit actions and read view-model state rather than directly re-implementing editing rules.
- Application code should orchestrate subsystems, not become the source of truth for document behavior.

## Application Layer

`src/app/application.c` is the bootstrap and frame-loop owner.

Supporting application modules narrow responsibilities:

- `application_runtime.c`
  Cursor state, viewport helpers, and `ToolEvent` construction.
- `application_file_actions.c`
  Save/load/export coordination.
- `application_dialog_actions.c`
  Dialog-facing status and confirmation flows.
- `command_dispatcher.c`
  Action-to-command bridge.
- `registration_manifest.c`
  Unified built-in registration sequencing.

## Registration Model

Built-in objects and tools are not hard-coded inside one monolithic runtime file.

They are assembled through manifests:

```text
workspace_init()
  -> app_register_all_manifests()
      -> object_manifest_register_all()
      -> tool_manifest_register_all()
```

Primary files:

- `src/app/extension_manifest.c`
- `src/app/tool_manifest.c`
- `src/app/registration_manifest.c`
- `src/app/manifest_runner.c`

Compatibility wrappers for older object-extension naming still exist in `include/app/extension_loader.h`, but new code should use the manifest naming.

## Current Editing Model

At a high level:

```text
input
-> tools or global commands
-> command execution or canvas update
-> document/session state
-> render + ui
```

Important consequences:

- Selection preview is session state.
- Final moves, creates, deletes, and property edits should be durable command operations.
- Rendering consumes state downstream; it does not define editor truth.

## Where to Read Next

- [core-systems.md](core-systems.md) for subsystem ownership
- [data-flow.md](data-flow.md) for event-to-render sequencing
- [extension-model.md](extension-model.md) for adding object types and tools
- [refactor-roadmap.md](refactor-roadmap.md) for the planned architecture cleanup sequence
