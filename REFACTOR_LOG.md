# Refactor Log

## 2026-05-25 - P2: Migrate Command Metadata Callers

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/input/input_router.c`,
  `src/ui/ui_menubar.c`,
  `src/ui/ui_menu_actions.c`,
  `src/app/editor_viewmodel.c`,
  `src/app/command_registry.c`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Updated input routing, menu UI, menu action emission, and view-model construction to query command metadata through `command_catalog` and command state through `command_availability`.
  Updated `command_registry.c` internals to call the extracted catalog/availability modules directly instead of routing through its own compatibility wrappers.
  Refreshed the roadmap pressure points and progress snapshot so it reflects completed workspace, UI frame, view-model, and render descriptor work.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - Architecture Refactor Roadmap Baseline

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `doc/architecture/refactor-roadmap.md`,
  `doc/architecture/overview.md`,
  `doc/README.md`
- Key changes:
  Added a current architecture refactor roadmap that sequences command registry cleanup, action handling consolidation, workspace boundary tightening, UI frame decomposition, view-model cleanup, and render scene descriptor work.
  Linked the roadmap from the architecture overview and documentation index.
- Validation:
  `cmake --build build --parallel` passed before documentation changes.
  `ctest --test-dir build --output-on-failure` passed before documentation changes with 11/11 tests passing.

## 2026-05-25 - P1: Extract Command Catalog Metadata

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/command_catalog.h`,
  `src/app/command_catalog.c`,
  `src/app/command_registry.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a dedicated command catalog module for stable command descriptors, menu ID lookup, command ID lookup, and dynamic tool command descriptor creation.
  Kept `command_registry_*` metadata lookup functions as compatibility wrappers while command execution and availability remain in `command_registry.c`.
  Added the new catalog source to the `editor_runtime` target and updated the file map.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extract Command Availability Rules

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/command_availability.h`,
  `src/app/command_availability.c`,
  `src/app/command_registry.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a dedicated command availability module for command executable-state checks, unavailable-reason strings, and menu-backed availability queries.
  Kept the existing `command_registry_is_available()`, `command_registry_unavailable_reason()`, and `command_registry_is_menu_action_available()` functions as compatibility wrappers.
  Reduced `command_registry.c` to command execution plus compatibility lookup/query wrappers for this slice.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extract Workspace Clipboard Operations

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_clipboard.h`,
  `src/app/workspace_clipboard.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a workspace clipboard module for selection copy, paste, and cut behavior, including paste offset handling and the cut transaction flow.
  Updated `command_registry_execute()` to delegate cut/copy/paste commands to the clipboard module.
  Added UI logic regression coverage for copy/paste/cut behavior and for cut pruning locked-layer selections before deleting editable objects.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|command|workspace"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extract Workspace View Commands

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_view_commands.h`,
  `src/app/workspace_view_commands.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a workspace view command module for zoom in, zoom out, zoom-to-fit, and grid toggling.
  Updated `command_registry_execute()` to delegate view commands to the new module.
  Added UI logic regression coverage for zoom in/out, zoom-to-fit on an empty document, zoom-to-fit on a small object, and grid toggling.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|command|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Introduce Editor Action Handler

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/editor_action_handler.h`,
  `src/app/editor_action_handler.c`,
  `src/app/command_dispatcher.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added an editor action handler module that owns execution of `EditorAction` payloads against a workspace.
  Reduced `command_dispatcher.c` to dispatcher state, null checks, and delegation to the handler.
  Preserved existing command dispatcher API so UI action sinks and tests continue to use the same entry point.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|command|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Introduce Editor Controller Facade

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/editor_controller.h`,
  `src/app/editor_controller.c`,
  `include/app/application_runtime.h`,
  `src/app/application_runtime.c`,
  `src/app/application.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a workspace-level editor controller facade for tool context creation, pointer capture checks, pointer event dispatch, scroll handling, pointer-anchor synchronization, tool event construction, and render scene snapshots.
  Migrated `application.c` pointer callbacks and render submission setup away from direct `app->workspace.core.*` / `app->workspace.session.*` access for those paths.
  Added UI logic regression coverage for editor-controller tool event construction and render scene snapshot creation.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extend Editor Controller Canvas Access

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/editor_controller.h`,
  `src/app/editor_controller.c`,
  `src/app/application_runtime.c`,
  `src/app/application_file_actions.c`,
  `tests/test_ui_logic.c`
- Key changes:
  Added controller APIs for canvas viewport updates, canvas background updates, canvas content-bounds queries, and read-only canvas access for export.
  Migrated resize and PNG export paths away from direct application-layer access to `Workspace` internals for those canvas operations.
  Added UI logic regression coverage for the new canvas controller APIs.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|workspace|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Input Router Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/application.c`,
  `src/input/input_router.c`
- Key changes:
  Updated `input_router.c` to use public workspace accessors for keymap lookup and tool-controller key dispatch instead of including `workspace_internal.h`.
  Added `workspace_set_service_callbacks()` so application startup can register save/load/export/action callbacks without directly writing the internal `workspace.services` fields.
  Replaced the direct service-field assignments in `application.c` with the new workspace registration API.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|workspace|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Command Availability Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/command_availability.c`
- Key changes:
  Added read-only workspace accessors for document, command executor, selection, clipboard count, and service availability checks.
  Updated command availability rules to use the public workspace query surface instead of `workspace_internal.h`.
  Kept the command availability behavior unchanged while moving internal layout knowledge back behind `workspace.c`.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command|workspace"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Editor ViewModel Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/editor_viewmodel.c`
- Key changes:
  Added const workspace accessors for canvas, tool controller, and keymap so read-only UI model building can stay outside the internal workspace layout.
  Updated `editor_viewmodel.c` to build command, tool, property, layer, dialog, and summary views through public workspace accessors instead of `workspace_internal.h`.
  Preserved the existing view-model output shape and UI-facing API while reducing direct `Workspace` field coupling.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|workspace|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Editor Action Handler Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/editor_action_handler.c`
- Key changes:
  Updated action dispatch to use public workspace accessors for document, command executor, selection, and tool-controller access instead of `workspace_internal.h`.
  Replaced direct dialog-state inspection with `workspace_active_dialog_kind()`.
  Added a local command-execution helper to keep repeated command/executor/document dispatch logic in one place while preserving existing action behavior.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|workspace|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Command Registry Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/command_registry.c`
- Key changes:
  Added `workspace_execute_service()` so command execution can invoke save/export callbacks without direct access to `workspace.services`.
  Updated command registry selection pruning, delete, select-all, undo/redo, tool activation, Save As path lookup, and shortcut formatting to use public workspace accessors.
  Removed `workspace_internal.h` from `command_registry.c`, leaving the command registry dependent on workspace behavior rather than workspace layout.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command|workspace"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Editor Controller Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/editor_controller.c`
- Key changes:
  Updated the editor controller facade to use public workspace accessors for canvas, document, selection, layout, and tool-controller access.
  Removed `workspace_internal.h` from `editor_controller.c` while keeping the existing controller API and render-scene snapshot behavior unchanged.
  Left internal workspace layout access limited to workspace-owned modules and the application storage struct.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|renderer|workspace|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Make Application Hold an Opaque Workspace

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_service.h`,
  `src/app/workspace_service.c`,
  `src/app/registration_manifest.c`,
  `src/app/application_internal.h`,
  `src/app/application.c`,
  `src/app/application_runtime.c`,
  `src/app/application_file_actions.c`,
  `src/app/application_dialog_actions.c`,
  `tests/test_workspace_service.c`
- Key changes:
  Added `workspace_create()` / `workspace_destroy()` so application code can own `Workspace` through an opaque pointer instead of embedding the internal struct layout.
  Updated `Application` to store `Workspace*`, removing the final app-layer include of `workspace_internal.h` and adjusting application runtime/file/dialog paths to pass the workspace pointer directly.
  Made `app_register_all_manifests()` idempotent so multiple workspace lifetimes in the same process do not fail on duplicate global object/tool registration.
  Added workspace service coverage for opaque workspace allocation and destruction.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "workspace|ui_logic|registry|command|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Reuse Command Catalog in ViewModel Construction

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/editor_viewmodel.c`
- Key changes:
  Removed the duplicate static command ID/scope mapping from `editor_viewmodel.c`.
  Updated command view construction to read command IDs and key scopes from `command_catalog_find_by_command()`.
  Kept command availability and unavailable-reason lookup behavior unchanged.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Decompose UI Frame Build Helpers

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_runtime.c`
- Key changes:
  Split `ui_system_build()` internals into private helpers for modal state update, frame timing/theme polling, menu/theme request handling, inspector animation/rendering, and canvas-content bounds publishing.
  Kept Nuklear rendering calls, layout fields, theme behavior, context-menu behavior, and published layout semantics unchanged.
  Left helper functions in `ui_runtime.c` for this first pass to reduce risk before moving UI build concerns into separate files.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Move UI Frame Build Into Dedicated Module

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `CMakeLists.txt`,
  `src/ui/ui_runtime.c`,
  `src/ui/ui_frame.c`,
  `doc/reference/file-map.md`
- Key changes:
  Moved `ui_system_build()` and its private frame-composition helpers from `ui_runtime.c` into the new `ui_frame.c` module.
  Kept `ui_runtime.c` focused on UI lifecycle, action emission, input forwarding, rendering, and layout query APIs.
  Added `ui_frame.c` to the Nuklear UI target and file map.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|renderer|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Introduce Render Scene Descriptor

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/render/render_system.h`,
  `src/render/render_system.c`,
  `src/app/application.c`,
  `tests/test_renderer.c`
- Key changes:
  Added `RenderSceneDesc` as the render-system input snapshot for document, selection, canvas, selection preview, and overlay object state.
  Replaced the long `render_system_draw()` parameter list with a single scene descriptor pointer.
  Updated application frame submission and renderer regression tests to pass the descriptor.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "renderer|ui_logic|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Extract Render Scene Cache Key

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/render/render_system.c`
- Key changes:
  Replaced the individual cached document/selection/canvas fields with a `RenderSceneCacheKey` value.
  Added helpers to build and compare cache keys from `RenderSceneDesc`.
  Kept draw-list invalidation behavior unchanged while making the cache comparison path easier to audit.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "renderer|ui_logic|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-12 21:25:51 CST - Baseline Check

- Build system: CMake (`CMakeLists.txt` at repository root).
- Source layout: `src/`, `include/`, `doc/`, and existing `build/` directory match the proposed plan.
- Baseline validation: `cmake --build build --parallel` completed successfully before refactor work started.

## 2026-05-12 21:29:13 CST - P0: Fix RenderSystem Selection Cache Key

- Modified files:
  `include/model/selection.h`,
  `src/model/selection.c`,
  `src/render/render_system.c`,
  `tests/test_selection.c`,
  `tests/test_renderer.c`
- Key changes:
  Added `SelectionSet.revision` and incremented it only when the selected ID set actually changes.
  Extended `RenderSystem` cache invalidation to depend on both `selection_count` and `selection_revision`.
  Added regression coverage for stable revisions on no-op updates and for render invalidation when the selection content changes without changing selection count.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'selection|renderer'` passed.

## 2026-05-12 21:39:06 CST - P0: Harden Lua Script Runtime Boundary

- Modified files:
  `src/script/script_runtime_lua.c`,
  `scripts/star_wreath.lua`,
  `tests/test_script_runtime.c`,
  `CMakeLists.txt`
- Key changes:
  Replaced per-event `luaL_dofile()` execution with a one-time script load path that caches the compiled script per runtime instance.
  Moved script access onto a whitelist-style `gldraw` table (`document_add_object`, `undo`, `redo`, `selection_get_ids`) inside a dedicated environment instead of exposing command text execution and unrestricted globals.
  Limited the available standard libraries to base helpers plus `math`, `string`, and `table`, and added a scripting-only regression test target that checks safe-library exposure and one-time loading behavior.
- Validation:
  `cmake --build build --parallel` passed for the default configuration (`GLDRAW_ENABLE_SCRIPTING=OFF`).
  `ctest --test-dir build --output-on-failure -R 'selection|renderer'` passed after the P0 changes.
  `cmake -S . -B build-scripting -DGLDRAW_ENABLE_SCRIPTING=ON -DCMAKE_BUILD_TYPE=Release` could not complete on this machine because CMake could not find `LUA_LIBRARIES` / `LUA_INCLUDE_DIR`, so the scripting-enabled branch could not be compiled locally.

## 2026-05-12 21:54:08 CST - P1: Split Public Workspace API From Internal State

- Modified files:
  `include/app/workspace.h`,
  `include/app/workspace_internal.h`,
  `src/app/workspace.c`,
  `src/app/application_internal.h`,
  `src/app/application_file_actions.c`,
  `src/app/editor_viewmodel.c`,
  `src/app/workspace_service.c`,
  `src/app/workspace_actions.c`,
  `src/app/workspace_dialogs.c`,
  `src/app/command_dispatcher.c`,
  `src/app/command_registry.c`,
  `src/input/input_router.c`,
  `src/tools/script_tool_lua.c`,
  `src/tools/tool_select.c`,
  `src/tools/tool_shape.c`,
  `tests/test_registry.c`,
  `tests/test_ui_logic.c`,
  `tests/test_workspace_service.c`,
  `CMakeLists.txt`
- Key changes:
  Moved the concrete `Workspace`/`EditorCore`/`EditorSession`/`EditorServices` layout into `workspace_internal.h` and reduced the public `workspace.h` surface to public enums, dialog/layout types, and opaque-pointer API declarations.
  Replaced the old public inline implementations with compiled functions in `src/app/workspace.c`, including the existing status/layout/dirty/clipboard helpers plus new accessor entry points for core editor services.
  Updated internal runtime modules and tests that need concrete workspace storage or direct field access to include the internal header explicitly, removing accidental dependence on the public header layout.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'workspace|ui_logic|registry|selection|renderer'` passed.

## 2026-05-12 22:09:20 CST - P1: Replace ToolContext Workspace Access With Ports

- Modified files:
  `include/tools/tool.h`,
  `include/script/script_runtime.h`,
  `src/app/workspace.c`,
  `src/tools/tool_select.c`,
  `src/tools/tool_shape.c`,
  `src/tools/script_tool_lua.c`,
  `src/script/script_runtime_lua.c`,
  `tests/test_ui_logic.c`,
  `tests/test_script_runtime.c`
- Key changes:
  Added `ToolPorts` to `ToolContext` and introduced helper wrappers for command execution, undo/redo, selection preview updates, and dirty-flag sync so tools no longer require direct `Workspace*` access for runtime mutations.
  Wired `workspace_tool_context()` to publish concrete port implementations backed by the workspace command executor and session preview/dirty state.
  Updated built-in select and shape tools plus the optional Lua-backed script tool/runtime to use the new port callbacks instead of reaching into `Workspace` internals or `CommandExecutor` directly.
  Updated tests to build contexts through `workspace_tool_context()` and to provide explicit test ports for script runtime execution.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'ui_logic|workspace|registry|selection|renderer|command'` passed.
  The scripting-enabled build still could not be compiled locally because this machine does not have the required Lua development package discoverable by CMake.

## 2026-05-12 22:32:08 CST - P1: Seal Platform Window Leakage Behind Callbacks and Adapters

- Modified files:
  `include/platform/window.h`,
  `src/platform/window.c`,
  `src/platform/window_internal.h`,
  `src/app/application.c`,
  `src/ui/ui_runtime.c`,
  `src/ui/ui_system_internal.h`
- Key changes:
  Added platform-owned window event callback registration APIs plus wrappers for window sizing, close requests, timed waits, and shared time queries so application runtime code no longer calls GLFW window functions directly.
  Moved GLFW callback dispatch into the platform layer and backed it with an internal native-window registry, removing the need for application/user-pointer ownership on `GLFWwindow`.
  Added a small platform-owned Nuklear/GLFW adapter surface so `ui_runtime.c` can initialize, frame, render, and shut down Nuklear without including `window_internal.h` or touching raw `GLFWwindow*`.
  Updated application startup and loop code to register platform callbacks and use platform wrappers for framebuffer polling and close control.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'ui_logic|workspace|registry|selection|renderer|command'` passed.

## 2026-05-12 22:42:37 CST - P1: Reorganize CMake Targets Around Runtime Boundaries

- Modified files:
  `CMakeLists.txt`
- Key changes:
  Replaced the old coarse library split (`gldraw_model`, `gldraw_render_gl`, `gldraw_commands`, `gldraw_ui_nuklear`, `gldraw_app`) with a layered target graph centered on `editor_model`, `editor_commands`, `editor_tools`, `editor_runtime`, `render_core`, `render_glfw_gl`, `platform_glfw`, `ui_nuklear_glfw`, and `editor_app`.
  Moved command core, tool runtime, render core, GL backend, platform window implementation, UI runtime, and application shell sources into separate targets so their responsibilities match the architecture more closely and command/tool/runtime cycles are broken explicitly in the link graph.
  Updated executable and test link dependencies to target the new layer boundaries, while keeping compatibility aliases for the previous `gldraw_*` library names.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:09:14 CST - P2: Introduce Render Resource Management Layer

- Modified files:
  `include/render/buffer_pool.h`,
  `include/render/shader_manager.h`,
  `src/render/buffer_pool.c`,
  `src/render/render_device_gl.c`,
  `src/render/shader_manager.c`,
  `CMakeLists.txt`
- Key changes:
  Added a `ShaderManager` module that centralizes shader source loading, shader compilation, program linking, uniform lookup, and program destruction instead of keeping that lifecycle embedded in `render_device_gl.c`.
  Added a `BufferPool` module that owns GL vertex stream creation, attribute layout setup, buffer growth, and VAO/VBO teardown so transient backend resources are managed through one resource layer.
  Updated `render_device_gl.c` to consume the new shader and buffer managers, reducing the backend file to frame state, transform logic, clip handling, and draw submission orchestration.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:27:42 CST - P2: Replace Draw-List Hot-Path Heap Churn With Reusable Arena Storage

- Modified files:
  `include/render/render_arena.h`,
  `src/render/render_arena.c`,
  `include/render/canvas_drawlist.h`,
  `src/render/canvas_drawlist.c`,
  `tests/test_renderer.c`,
  `CMakeLists.txt`
- Key changes:
  Added a reusable `RenderArena` allocator module with explicit init/reset/shutdown/reserve/alloc operations so temporary render-build storage lives behind a dedicated abstraction instead of ad hoc per-call heap usage.
  Updated `CanvasDrawList` to own a build-scoped scratch arena and reset it once per build, making temporary object path buffers, grid geometry buffers, and visible-index arrays reuse retained memory across frames.
  Removed the hot-path `malloc/free` pairs from `canvas_drawlist_build()` and its helper paths, while keeping the externally visible draw-list output arrays and submission flow unchanged.
  Added a renderer regression test that verifies the draw-list scratch arena remains allocated and reusable across repeated builds.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:46:18 CST - P2: Raise Render Abstraction Around Pass, Material, and Geometry

- Modified files:
  `include/render/render_device.h`,
  `src/render/render_device.c`,
  `include/render/canvas_drawlist.h`,
  `src/render/canvas_drawlist.c`,
  `src/render/render_device_gl.c`,
  `src/render/canvas_renderer.c`,
  `tests/test_renderer.c`
- Key changes:
  Replaced the old immediate-mode render-device surface (`set_color`, `draw_path`, `draw_rect`, `draw_line`, mutable transform/clip state) with explicit `RenderPass`, `RenderMaterial`, and `RenderGeometry` value types.
  Updated `canvas_renderer_submit()` to build a pass object once per frame submission and then submit each stroke as a geometry/material pair, making the renderer intent explicit and reducing hidden backend state transitions.
  Simplified the GL backend so pass setup owns clip and transform state, while draw submission consumes geometry and material payloads directly instead of relying on prior setter calls.
  Updated renderer tests to validate the new frame/pass/draw submission sequence and geometry metadata capture.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:54:03 CST - P3: Future Architecture Evolution Suggestions

- Render graph direction:
  If GLDraw grows beyond a single canvas line-render pass, evolve the current frame/pass model into a render graph so offscreen composition, post-processing, and dependency ordering are explicit instead of being encoded inside backend control flow.
- Backend portability direction:
  Split backend-facing rendering into a `RenderBackend` layer plus a separate context/provider abstraction so OpenGL-specific context creation assumptions stop leaking into the rendering lifecycle and alternate backends can be introduced incrementally.
- Editor ecosystem direction:
  Keep the current static manifest model unless there is a real plugin-distribution goal; if that goal appears later, introduce a narrow plugin ABI centered on registration APIs rather than exposing workspace or renderer internals directly.
