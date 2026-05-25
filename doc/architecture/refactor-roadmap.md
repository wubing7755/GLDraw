# Architecture Refactor Roadmap

> Audience: maintainers, contributors doing structural work
> Purpose: sequence the next architecture cleanup without changing editor behavior accidentally
> Source of truth: current source tree, `REFACTOR_LOG.md`, and the architecture pages in this directory
> Last reviewed with code: 2026-05-25
> Related: [overview.md](overview.md), [core-systems.md](core-systems.md), [data-flow.md](data-flow.md)

## Intent

GLDraw already has useful runtime boundaries: `Workspace`, descriptor-driven objects and tools, command-based undo, view-model driven UI, and a render-device abstraction.

The next refactor should not reduce layering for its own sake. It should turn passive forwarding layers into clear ownership boundaries, reduce direct access to `Workspace` internals, and make user actions flow through fewer, more predictable entry points.

## Current Pressure Points

- `src/app/command_registry.c` mixes command metadata, availability checks, command execution, clipboard behavior, dialog opening, view commands, and tool activation.
- `src/app/command_dispatcher.c` overlaps with `command_registry.c`; some UI actions execute through the registry while others create commands directly.
- `src/app/application.c` and helper modules still reach into `app->workspace.core` and `app->workspace.session`, so the public `Workspace` boundary is not fully enforced.
- `src/ui/ui_runtime.c` builds menu, layout, animation, panels, dialogs, context menus, theme reload, and layout publishing in one large frame function.
- `src/app/editor_viewmodel.c` includes `workspace_internal.h` and duplicates command mapping data that should belong to the command catalog.
- `render_system_draw()` takes a long parameter list where a render-scene descriptor would communicate intent better.

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

## Per-Step Completion Criteria

- The project builds.
- Relevant tests pass; full CTest should pass after every non-trivial step.
- Behavior is unchanged unless the step explicitly fixes a bug.
- Diffs stay focused on one architectural move.
- `REFACTOR_LOG.md` records modified files, key changes, and validation commands.

## Suggested First Implementation Slice

Start with command catalog extraction. It has the best risk-to-benefit ratio because it reduces pressure on `command_registry.c` without changing command execution behavior.
