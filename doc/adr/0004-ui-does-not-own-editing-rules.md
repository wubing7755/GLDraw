# ADR 0004: UI Does Not Own Editing Rules

## Status

Accepted.

## Context

GLDraw's UI is built with Nuklear and split across focused `src/ui/` modules.
Editing behavior must remain consistent across menus, shortcuts, context menus,
dialogs, tools, and future automation.

## Decision

UI code should present state and dispatch actions. Editing rules belong in
workspace commands, tool runtime, document modules, or services depending on the
behavior.

## Consequences

* UI modules should consume workspace query state, view-model data, or command
  availability.
* Menu items, toolbar controls, dialogs, and context menus should converge on the
  same command/action paths.
* Business rules should not be hidden inside widget event branches.
* UI tests should focus on state derivation and action routing rather than
  duplicating document or command tests.
