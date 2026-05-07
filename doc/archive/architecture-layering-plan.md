# Architecture Layering Plan

> Status: historical design note
> Purpose: preserve the rationale behind the layering refactor without treating it as the current source of truth
> Current source of truth: [../architecture/overview.md](../architecture/overview.md)

## Historical Context

This note captures the direction of the layering refactor that introduced the explicit `Workspace -> EditorCore / EditorSession / EditorServices` split.

It is kept for historical context, not for day-to-day implementation guidance.

## What Still Matters

These architectural outcomes still survive in the current codebase:

- persisted document data is separated from runtime-only session state
- application/UI code reaches editor state through workspace-owned layers
- editing history is handled by a dedicated undo/redo subsystem instead of being mixed into UI state
- incremental subsystem extraction is preferred over monolithic rewrites

## What Changed Since This Plan

- `CommandExecutor` is the current undo/redo owner in `EditorCore`
- current extension guidance is descriptor- and manifest-based
- current implementation details should be read from:
  - [../architecture/overview.md](../architecture/overview.md)
  - [../architecture/core-systems.md](../architecture/core-systems.md)
  - [../architecture/data-flow.md](../architecture/data-flow.md)

## Why Keep This File

This file is useful when you need to understand why the workspace split exists or why some compatibility wrappers remain in the codebase.

It should not be cited as the definitive description of today's command, object, or tool architecture.
