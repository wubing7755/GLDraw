# Architecture Refactor Roadmap

> Audience: maintainers, contributors doing structural work
> Purpose: sequence the next architecture cleanup without changing editor behavior accidentally
> Source of truth: current source tree, `REFACTOR_LOG.md`, and the architecture pages in this directory
> Last reviewed with code: 2026-05-29
> Related: [overview.md](overview.md), [core-systems.md](core-systems.md), [data-flow.md](data-flow.md)

## Intent

GLDraw already has useful runtime boundaries: `Workspace`, descriptor-driven objects and tools, command-based undo, view-model driven UI, and a render-device abstraction.

The next refactor should not reduce layering for its own sake. It should turn passive forwarding layers into clear ownership boundaries, reduce direct access to `Workspace` internals, and make user actions flow through fewer, more predictable entry points.

## Current Pressure Points

- `src/app/command_registry.c` now owns the public command execution entry point and routes concrete behavior through a static command execution table.
- `command_registry_execute()` remains the execution entry point, while command metadata and availability callers now use `command_catalog` and `command_availability` directly.
- File command behavior now lives in `workspace_file_commands`, leaving the registry to delegate New/Open/Save/Save As/Export/Exit commands.
- Help and modal dialog command behavior now lives in `workspace_dialog_commands`.
- Undo, redo, delete selection, and select-all behavior now lives in `workspace_edit_commands`.
- Dynamic tool activation now lives in `workspace_tool_commands`.
- View-model construction now captures workspace state into a build context and emits summary, command, tool, layer, property, and dialog snapshots through separate builders.
- UI frame construction has moved to `src/ui/ui_frame.c`, the inspector's layer controls now live in `src/ui/ui_layer_panel.c`, menu bar rendering lives in `src/ui/ui_menubar_render.c`, and context menu rendering lives in `src/ui/ui_context_menu_render.c`.
- Render submission now uses `RenderSceneDesc`; render-system internals capture that descriptor into a scene snapshot/cache-key helper before deciding whether to reuse the draw list.
- Canvas draw-list strokes are normalized to line-segment geometry and `canvas_renderer_submit()` batches adjacent strokes with identical material and primitive metadata before backend submission.
- Bundled shader, theme, and script paths now resolve through a base resource-path helper instead of assuming the process current working directory is the repository root.

## Progress Snapshot

- Command descriptors live in `command_catalog`.
- Command identifiers and descriptor value types live in `command_types`.
- Command executable-state checks live in `command_availability`.
- Command metadata and availability compatibility wrappers have been removed from `command_registry`.
- Command execution routing in `command_registry` uses a static table for fixed commands, with dynamic tool activation delegated separately.
- File/service commands have a dedicated workspace command module.
- Help/modal commands have a dedicated workspace dialog command module.
- Undo/redo and selection edit commands have a dedicated workspace edit command module.
- Dynamic tool commands have a dedicated workspace tool command module.
- Clipboard and view commands have dedicated workspace modules.
- Editor actions dispatch through `editor_action_handler`.
- Application code owns an opaque `Workspace*` instead of embedding workspace internals.
- Outer app/input/UI/view-model/controller layers no longer include `workspace_internal.h`.
- `input_router.c` now consumes public workspace/keymap APIs and routes tool key events through `editor_controller`, keeping keyboard routing outside the concrete workspace storage layout.
- Built-in tools now consume `ToolPointerButton`, `ToolInputModifier`, and `ToolInputKey` values from `include/tools/tool.h`; GLFW button/key/modifier values are mapped at the application/input boundary.
- `EditorViewModel` construction is split by view concern behind the stable public view-model shape.
- `ui_system_build()` lives in `ui_frame.c`.
- Inspector selection/property rendering and layer controls are split across `ui_inspector_panel.c` and `ui_layer_panel.c`.
- Menu bar state/lifecycle and Nuklear rendering are split across `ui_menubar.c` and `ui_menubar_render.c`.
- Context menu lifecycle/input handling and Nuklear item rendering are split across `ui_context_menu.c` and `ui_context_menu_render.c`.
- Theme built-in descriptors and token presets live in `ui_theme_builtin.c`, with private theme-system declarations in `ui_theme_internal.h`.
- External theme registry state, file/directory reload, reload errors, and directory signatures live in `ui_theme_external.c`.
- Theme JSON extraction, color parsing, path-derived IDs, override application, and token clamping live in `ui_theme_parse.c`.
- Selected theme ID settings load/save lives in `ui_theme_settings.c`.
- Nuklear stylesheet mapping for theme tokens lives in `ui_theme_apply.c`.
- Document persistence JSON tokenization, skipping, and primitive parsing live in `persistence_json.c/.h`.
- Document layer persistence parsing lives in `persistence_layers.c/.h`.
- Document object persistence parsing and loaded-object construction live in `persistence_objects.c/.h`.
- Document JSON writing lives in `persistence_write.c`.
- Object command pre-execution validation lives in `command_object_checks.c`.
- Create, delete-selection, move, paste, and set-property object command implementations live in separate command source files.
- `Document` storage layout now lives in `include/document/document_internal.h`; public callers use the opaque `Document` API plus object/layer query functions.
- Platform window callbacks live in `application_callbacks.c`, leaving registration in the application lifecycle.
- Platform window event dispatch now fans out key, character, mouse-button, and scroll input to Nuklear before application callbacks, keeping immediate-mode UI input ownership in the platform adapter rather than in application code.
- Application-owned workspace save/load/export and workspace action callbacks live in `application_workspace_services.c`.
- `application.c` is scoped to lifecycle setup, shutdown, and frame sequencing.
- `render_system_draw()` consumes `RenderSceneDesc`.
- `RenderSceneDesc` lives in `include/render/render_scene.h`, while scene cache-key capture and comparison live behind the render-system implementation boundary.
- Resource lookup for bundled files lives in `base/resource_path`, with OpenGL shader loading, UI theme directory reloads, and optional script tools resolving the same installed/development search paths.
- CMake installs bundled `shaders`, `themes`, and `scripts` under `share/gldraw`, and the GLFW FetchContent declaration pins the downloaded archive with `URL_HASH`.
- This refactor round is complete for theme modules, document persistence modules, object command modules, application lifecycle boundaries, input/document/render boundary tightening, and bundled resource/build layout hardening.

## Compatibility Notes

- `include/app/extension_loader.h` keeps object-extension registration wrappers so older extension registration call sites can continue to build while the manifest API remains the current owner.
- `src/tools/tool_registry.c` keeps `register_builtin_tools()` as a compatibility entrypoint while the built-in tool manifest remains the preferred registration path.
- `glad` and `nuklear` TODO/compatibility audit hits are generated or vendored third-party code and are not GLDraw architecture shims.

## Boundary Rules

These rules are the target state for the refactor:

- `workspace_internal.h` should be used by workspace implementation files and narrowly scoped runtime internals only.
- UI code should consume `EditorViewModel` and emit `EditorAction`; it should not query command registries or workspace internals directly for business rules.
- Application code should own process lifetime, platform callbacks, file pickers, and frame order; it should not be the source of document editing behavior.
- Durable document edits should still go through `CommandExecutor`.
- New editor behavior should have one obvious execution owner, not separate menu, shortcut, and inspector implementations.
- Render code should consume an editor/render scene snapshot; it should not query unrelated application state.

## Step Plan

### 1. Establish Baseline

- Build with `cmake --build build --parallel`.
- Run `ctest --test-dir build --output-on-failure`.
- Record the result in `REFACTOR_LOG.md`.

### 2. Split Command Metadata From Behavior

- Add a command catalog module for command descriptors, IDs, menu IDs, scopes, and dynamic tool command mapping.
- Keep compatibility wrappers in `command_registry.c` while callers migrate.
- Do not move command behavior in the same step.

### 3. Split Command Availability

- Move command availability and unavailable-reason logic into a dedicated module.
- Make view-model and menu code read command state through this module or through the view model.
- Keep behavior unchanged.

### 4. Extract Clipboard Behavior

- Move copy, paste, cut, clipboard reservation, and paste-offset logic out of `command_registry.c`.
- Keep clipboard storage in `EditorSession` until a later step proves a separate owner is useful.
- Add or extend tests for empty selection, locked layers, cut rollback, and paste selection.

### 5. Extract View Commands

- Move zoom in, zoom out, zoom fit, and grid toggling into a small view-command module.
- Add focused coverage around empty documents, very small bounds, and viewport fallback behavior.

### 6. Unify Action Handling

- Introduce an editor action or command handler layer that owns property edits, layer operations, menu commands, and shortcut commands.
- Reduce `command_dispatcher.c` to action decoding and handler dispatch.
- Avoid adding new UI-specific execution paths.

### 7. Add An Editor Runtime Facade

- Introduce a facade around keyboard, pointer, action execution, view-model build, and render-scene build.
- Move direct `app->workspace.core` and `app->workspace.session` access out of `application.c`.
- Keep `Application` focused on platform lifecycle and frame sequencing.

### 8. Tighten Workspace Internal Access

- Replace direct `workspace_internal.h` includes in input routing and view-model building with public query APIs or the editor runtime facade.
- Use `rg "#include <app/workspace_internal.h>" src include tests` as the recurring check.
- Tests may keep internal access only when they are explicitly verifying internal lifecycle behavior.

### 9. Decompose UI Frame Build

- Split `ui_system_build()` into private helpers for layout, menubar, tool rail, inspector, status bar, dialogs, context menu, and layout publishing.
- Keep Nuklear-specific code inside the UI implementation boundary.
- Preserve current visual behavior.

### 10. Clean Up ViewModel Construction

- Move duplicated command mapping data to the command catalog.
- Build the view model through read-only editor queries rather than direct workspace field reads.
- Consider incremental allocation reuse only after the ownership cleanup is complete.

### 11. Use A Render Scene Descriptor

- Replace the long `render_system_draw()` parameter list with a `RenderSceneDesc`.
- Extract render cache-key comparison into a helper or value struct.
- Keep the draw-list and backend contracts stable during the first pass.

### 12. Remove Compatibility Shims

- Delete temporary wrappers after call sites have migrated.
- Update architecture docs and file maps with the final ownership map.
- Run the full build and test suite after each cleanup.

### 13. Keep Command Execution Routing Thin

- Keep fixed command routes in the command execution table and concrete behavior in workspace owner modules.
- Keep `command_registry_execute()` as the public execution entry point until the replacement action executor API is stable.
- Prefer focused regression tests around each route when adding or moving command behavior.

## Per-Step Completion Criteria

- The project builds.
- Relevant tests pass; full CTest should pass after every non-trivial step.
- Behavior is unchanged unless the step explicitly fixes a bug.
- Diffs stay focused on one architectural move.
- `REFACTOR_LOG.md` records modified files, key changes, and validation commands.

## Suggested Next Implementation Slice

The render scene snapshot ownership slice is complete. Future work should be planned as a separate slice, starting with GL/Nuklear backend lifecycle hardening or the larger theme system only after adding focused tests around the target area.
