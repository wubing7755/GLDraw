# ADR 0002: CommandExecutor for Durable Mutations

## Status

Accepted.

## Context

GLDraw supports undo and redo. Durable document changes need consistent history,
dirty-state, selection, and persistence behavior.

## Decision

Durable document mutations should go through `CommandExecutor`. Commands are the
unit of undoable editing behavior.

## Consequences

* Tools and UI actions should emit commands for durable edits.
* Direct document mutation is appropriate only for document-owned internals,
  loading, tests, or narrowly scoped setup code.
* Command changes should include tests for success paths, unavailable states, and
  undo/redo behavior when document state changes.
* Preview and overlay state may remain outside command history until committed.
