# Refactor Log

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
