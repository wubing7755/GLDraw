# Architecture Layering Plan

This note captures the incremental architecture direction implemented as part of issue #55.

## Current layering

The editor runtime is now split into three explicit layers in `Workspace`:

- `EditorCore`
  - `Document`
  - `DocumentHistory`
  - `CanvasView`
  - `ToolController`
- `EditorSession`
  - selection state
  - clipboard state
  - current file path
  - dirty/save tracking
  - keymap
  - layout and dialog state
  - status text
- `EditorServices`
  - save/load callbacks
  - deferred action executor
  - application-owned callback context

This keeps persisted drawing data separate from runtime/editor-only state and makes backend integration points explicit.

## Boundaries clarified in this refactor

### Persisted document vs session-only state

- `Document` now owns only persisted drawing data and revision bookkeeping.
- Selection is no longer stored on `Document`.
- Selection now lives in `EditorSession`, which matches actual runtime behavior and avoids re-entangling persistence, rendering, and UI state.

### Self-contained history

- `DocumentHistory` now owns all of its auxiliary entry-kind and lightweight edit buffers directly.
- The previous global internal registry was removed.
- Undo/redo now restores both document snapshots and session selection state through explicit parameters.

### Command path consistency

- Menu and shortcut actions continue to route through `command_registry_execute()`.
- Session-sensitive edit commands now operate on `EditorSession.selection` rather than hidden document state.

### Backend boundaries

- Application/UI code now reaches editing state through `workspace.core`, `workspace.session`, and `workspace.services`.
- This reduces the amount of backend-specific code that needs knowledge of mixed editor state.

## Incremental follow-up slices

1. Extract `EditorCore` lifecycle helpers from `application.c`.
2. Move clipboard and file/session actions behind a dedicated session/service module instead of `command_registry.c`.
3. Introduce a renderer-facing scene struct so `render_system_draw()` does not depend directly on raw document storage.
4. Isolate Nuklear-specific UI implementation details behind a narrower presenter/model boundary.
5. Convert remaining tool and inspector edit flows to use explicit command objects for all user-visible edits.
