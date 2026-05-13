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
