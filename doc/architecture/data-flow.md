# Data Flow

> Audience: contributors, maintainers
> Purpose: explain how startup, input, command execution, persistence, and rendering connect
> Source of truth: `src/app/application.c`, `src/app/application_runtime.c`, `src/tools/`, `src/commands/`, `src/render/`
> Last reviewed with code: 2026-05-07
> Related: [overview.md](overview.md), [core-systems.md](core-systems.md)

## Startup and Registration

The runtime starts small and then registers built-ins through manifests.

```text
main()
  -> app_run()
  -> app_init()
  -> workspace_init()
  -> app_register_all_manifests()
      -> object_manifest_register_all()
      -> tool_manifest_register_all()
  -> ui_system_create()
  -> render_system_create()
```

Notes:

- object and tool registration are explicit startup steps
- legacy object-loader names are compatibility wrappers, not the preferred model

## Frame Loop

The frame loop is intentionally ordered so the UI and renderer consume a coherent workspace snapshot.

```text
poll events
-> begin UI frame
-> build editor view model
-> build UI
-> sync canvas viewport
-> draw render scene
-> flush pending export if requested
-> render UI
-> swap buffers
```

## Pointer and Keyboard Input

Platform callbacks land in the application layer first.

```text
GLFW callback
  -> application runtime helper
  -> UI hit test / UI handler
  -> ToolEvent construction or key routing
  -> tool controller or command registry
  -> command execution or canvas update
```

Key details:

- pointer input is filtered through the UI before it reaches tools, unless a tool already owns the interaction
- wheel input zooms through the canvas-aware tool input path
- global shortcuts go through the keymap and command registry path

## Tool Interaction Flow

Typical tool lifecycle:

```text
pointer down
-> tool-local state update
-> optional preview / overlay state
-> pointer move updates preview
-> pointer up commits durable change if needed
```

Examples:

- shape tools keep a temporary overlay object during drag, then create a real object through a command at commit time
- the select tool keeps selection movement preview in session state, then commits a move command on release

## Durable Edit Flow

User-visible document edits should converge on the command layer.

```text
tool or UI action
  -> command creation
  -> command_executor_execute()
  -> Document mutation
  -> dirty-state sync
  -> render + UI reflect new state
```

This is what keeps undo/redo, dirty tracking, and behavior consistency aligned.

## Persistence Flow

Save/load is coordinated by the application layer but executed through workspace and document services.

```text
menu / shortcut / dialog request
  -> application file action helper
  -> workspace service callback
  -> document_save_json() / document_load_json()
  -> runtime-only session/tool/history reset as needed
```

Notes:

- persistence and undo history are separate concerns
- runtime-only state such as current selection or tool previews may need reset after load

## Render Flow

Rendering stays downstream from editing logic.

```text
render_system_draw()
  -> consume document + selection + canvas state
  -> query visible objects
  -> build draw data
  -> draw geometry
  -> draw selection preview and tool overlay
```

The renderer should reflect editor state, not define it.
