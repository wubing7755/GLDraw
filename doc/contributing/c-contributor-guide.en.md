# GLDraw C Contributor Guide

> Audience: developers who can read basic C and want to modify GLDraw
> Purpose: explain GLDraw-specific patterns and edit paths, not generic C syntax
> Source of truth: current source tree and `doc/architecture/`
> Last reviewed with code: 2026-05-07
> Related: [overview.md](overview.md), [../architecture/extension-model.md](../architecture/extension-model.md)

## What this guide is for

This is not a general C tutorial.

It focuses on three things:

1. The abstractions that actually matter in GLDraw.
2. Where different kinds of changes should start.
3. How to avoid changes that compile but violate the intended edit model.

## Build the right mental model first

Read these before making non-trivial changes:

1. [../architecture/overview.md](../architecture/overview.md)
2. [../architecture/data-flow.md](../architecture/data-flow.md)
3. [../reference/file-map.md](../reference/file-map.md)
4. `src/app/application.c`
5. `src/app/command_dispatcher.c`
6. `src/tools/tool_runtime.c`
7. `src/commands/command_executor.c`

The main path to understand is:

`GLFW callbacks -> application runtime helpers -> tool/input dispatch -> command execution or canvas update -> render + ui`

## Core abstractions you must understand

### `Workspace`

`Workspace` is the runtime container, split on purpose:

- `EditorCore` owns `Document`, `CommandExecutor`, `CanvasView`, and `ToolController`
- `EditorSession` owns selection, clipboard, layout, dirty state, current path, and status text
- `EditorServices` owns application-supplied callbacks for save/load/export flows

If data should persist in the document, it usually should not live only in `EditorSession`.

### `CommandExecutor`

Durable document changes should normally go through the command system.

It owns:

- undo/redo
- merge behavior
- transactions
- history memory budgeting

Direct `Document` mutation is often a bug unless you are inside a command implementation or a tightly controlled setup path.

### `GraphicObjectDescriptor`

Object extensibility is descriptor-based, not driven by one large central `switch`.

The current model is:

- each object type implements callbacks
- registration happens through `register_object_type()`
- built-ins are assembled in `src/app/extension_manifest.c`

Start with:

- `include/document/object.h`
- `src/document/object_registry.c`
- `src/document/object_runtime.c`
- `src/document/object_line.c`
- `src/document/object_rect.c`
- `src/document/object_ellipse.c`
- `src/document/object_fake_star.c`

### `ToolDescriptor`

Tools follow the same descriptor model.

The current model is:

- each tool defines create, activate, pointer, key, and overlay callbacks
- registration happens through `register_tool()` or `register_shape_tool()`
- built-ins are assembled in `src/app/tool_manifest.c`

Start with:

- `include/tools/tool.h`
- `src/tools/tool_registry.c`
- `src/tools/tool_runtime.c`
- `src/tools/tool_select.c`
- `src/tools/tool_pan.c`
- `src/tools/tool_shape.c`

### `CanvasView`

`CanvasView` provides spatial context, not durable editing semantics.

It owns:

- world/screen transforms
- viewport
- zoom and pan
- picking-related view helpers

If behavior looks spatially wrong, inspect `canvas/` before changing tools or rendering.

## Where to start common changes

### Add a new object type

Recommended path:

1. Use `src/document/object_line.c` or `src/document/object_fake_star.c` as a reference.
2. Implement the payload and descriptor callbacks in a new file.
3. Register it with `register_object_type()`.
4. Wire it into `src/app/extension_manifest.c`.
5. Add a tool only if the object needs its own interaction flow.
6. Cover serialization, hit testing, property schema, and tests.

Do not fall back to the older pattern of adding branches in a shared `object.c`.

### Add a new tool

Recommended path:

1. Decide whether `register_shape_tool()` already covers it.
2. Otherwise use `src/tools/tool_select.c` or `src/tools/tool_pan.c` as a reference.
3. Implement a `ToolDescriptor`.
4. Register it in `src/app/tool_manifest.c`.
5. Check shortcut, command ID, tooltip, and icon text.
6. Verify activation, pointer capture, overlay behavior, and the command path.

### Change inspector or menu behavior

Start with:

- `src/ui/ui_inspector_panel.c`
- `src/ui/ui_chrome.c`
- `src/ui/ui_menu_def.c`
- `src/app/command_dispatcher.c`
- `src/app/command_registry.c`

Rule of thumb:

- UI should emit actions or command requests.
- Business semantics should stay in workspace, command, or tool layers.
- Durable user-visible edits should not stop at temporary UI state.

### Change shortcuts

Start with:

- `src/input/keymap.c`
- `src/ui/ui_menu_def.c`
- `default_shortcut` values in `src/tools/tool_*.c`
- [../user/controls.md](../user/controls.md)

## Common pitfalls

### Pitfall 1: Treating preview state as committed state

Selection dragging uses session preview state during the gesture and commits a move command on release. Writing into `Document` during the preview path usually breaks undo/redo and interaction consistency.

### Pitfall 2: Changing rendering without changing the model

Symptoms:

- the frame looks right
- save/load loses data
- hit testing or selection becomes inconsistent

That usually means the render path changed but `Document`, descriptors, or persistence did not.

### Pitfall 3: Changing the model without updating registration

A new object or tool may compile and still never become reachable if the manifest was not updated.

### Pitfall 4: Reading historical docs as current architecture

Use these as the current docs:

- `doc/architecture/overview.md`
- `doc/architecture/extension-model.md`
- `doc/reference/file-map.md`

## Pre-PR baseline

1. Build locally.
2. Walk the user flow you changed.
3. If you changed an object or tool, verify registration works.
4. If you changed shortcuts, doc structure, or extension entrypoints, update `doc/`.
5. If you changed command execution paths, verify undo/redo still works.
