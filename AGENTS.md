# AGENTS.md

This file gives Codex repository-specific guidance for GLDraw.

## Required Reading For AI Agents

Before making changes, read:

* `CONTRIBUTING.md` - contributor workflow, PR expectations, and architecture
  boundaries.
* `doc/ai-agents.md` - AI agent policy, restrictions, review, and disclosure
  rules.
* `doc/agent-playbooks.md` - task-specific AI workflows.
* `doc/testing.md` - test layers, regression expectations, and manual
  verification.

For architecture-sensitive work, also read:

* `doc/adr/README.md`
* Relevant ADR files in `doc/adr/`

For build, CI, packaging, release, dependency, or security work, also read:

* `doc/build.md`
* `doc/dependencies.md`
* `doc/release.md`
* `SECURITY.md` when dependency or vulnerability handling is involved.

## Project Shape

GLDraw is a C11 canvas drawing editor built with CMake, GLFW, GLAD, Nuklear,
and OpenGL 3.3. The runtime is centered on `Workspace`:

```text
Workspace
  -> EditorCore: Document, CommandExecutor, CanvasView, ToolController
  -> EditorSession: keymap, layout, selection, clipboard, dialogs, dirty state
  -> EditorServices: save, load, export, and top-level action callbacks
```

Treat `Workspace` as the editor hub, but prefer public workspace and controller
APIs over direct access to internals.

## Build And Verification

Use the scripts for normal local builds:

```sh
./build.sh
./build.sh debug
./build.sh clean
```

On Windows CMD with MinGW/MSYS2:

```bat
build.bat
build.bat debug
build.bat clean
```

Manual CMake references:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

```bat
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
```

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Dependencies: GLFW 3.3.9 is fetched by CMake, GLAD is committed, and Nuklear is
header-only. Tests are expected mainly in Linux/macOS CI.

## Current Entry Points

Start code reading here:

* `src/main.c` - process entrypoint, calls `app_run()`.
* `src/app/application.c` - application lifecycle and frame sequencing.
* `src/app/application_callbacks.c` - platform callback registration targets.
* `src/app/application_workspace_services.c` - app-owned save/load/export callbacks.
* `include/app/workspace.h` - public workspace API.
* `include/app/editor_controller.h` - editor runtime facade for input and render scene state.
* `src/app/registration_manifest.c` - built-in registration sequence.
* `src/app/extension_manifest.c` - built-in object registration.
* `src/app/tool_manifest.c` - built-in tool registration.
* `src/app/command_registry.c` - public command execution entrypoint.
* `src/commands/command_executor.c` - undo/redo history and command execution.
* `src/render/render_system.c` - render system lifecycle and scene submission.
* `src/tools/tool_runtime.c` - active tool lifecycle and dispatch.
* `CMakeLists.txt` - CMake project entrypoint.
* `cmake/` - CMake modules for compiler options, dependencies, sources, tests,
  and packaging.

## Ownership Rules

Keep edits aligned with the current boundaries:

* Durable document mutations should go through `CommandExecutor`.
* Object storage, layers, IDs, revisions, and spatial queries belong to `document/`.
* Selection, clipboard, dialogs, dirty state, and layout are workspace/session concerns.
* Tool input should flow through `editor_controller` and `tool_runtime`, not directly from UI widgets.
* UI code should consume view-model or workspace query state and emit actions; it should not own editing rules.
* Rendering consumes `RenderSceneDesc` or editor render scene state; it should not define document behavior.
* Platform-specific input values should be converted at the app/input boundary before reaching tools.
* Resource lookup for shaders, themes, and scripts should use `base/resource_path`.

Avoid adding new includes of `workspace_internal.h` outside workspace implementation files unless a test is explicitly verifying internal lifecycle behavior.

## Extension Patterns

Objects are descriptor-driven through `GraphicObjectDescriptor` in
`include/document/object.h`.

* Implement create, clone, destroy, translate, bounds, hit-test, path, property,
  and persistence callbacks as needed.
* Register object descriptors with `register_object_type()`.
* Add built-in object registration through `src/app/extension_manifest.c`.
* Prefer `GRAPHIC_OBJECT_INVALID` for new auto-assigned object type IDs.

Tools are descriptor-driven through `ToolDescriptor` in `include/tools/tool.h`.

* Register custom tools with `register_tool()`.
* Use `register_shape_tool()` when the standard shape-drag lifecycle is enough.
* Add built-in tool registration through `src/app/tool_manifest.c`.
* Keep durable edits command-based; overlays are for previews.

## Command And UI Notes

Command metadata, availability, and execution are split:

* `command_catalog` owns command descriptors and IDs.
* `command_availability` owns executable-state checks.
* `command_registry_execute()` remains the public execution entrypoint.
* Concrete workspace behavior is split across `workspace_file_commands`,
  `workspace_edit_commands`, `workspace_view_commands`,
  `workspace_tool_commands`, and `workspace_dialog_commands`.

UI frame construction is split across focused `src/ui/` modules such as
`ui_frame.c`, `ui_chrome.c`, `ui_menubar*.c`, `ui_context_menu*.c`,
`ui_inspector_panel.c`, `ui_layer_panel.c`, and dialog/theme modules.

## Documentation

Repository-local docs are intentionally short:

* `doc/README.md` - documentation entrypoint.
* `doc/build.md` - build, run, dependencies, and troubleshooting.
* `doc/controls.md` - default shortcuts and pointer behavior.
* `doc/dependencies.md` - dependency locations and update policy.
* `doc/testing.md` - testing policy and expectations.
* `doc/ai-agents.md` - AI agent policy.
* `doc/agent-playbooks.md` - AI-assisted task workflows.
* `doc/archive/` - historical notes retained for reference only.

Architecture and source navigation are maintained through Zread:
https://zread.ai/wubing7755/GLDraw

Update local docs when build commands, run paths, dependencies, controls,
testing policy, release policy, AI agent policy, or documentation policy change.
