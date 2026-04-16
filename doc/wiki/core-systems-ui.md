# UI System

## Overview

`UiSystem` uses Nuklear to build editor chrome around the canvas.

Current panels:

- top app bar (menu + quick actions)
- left tool rail
- right inspector
- bottom status strip

## Responsibilities

- switch the active tool
- expose New/Open/Save and undo/redo/zoom quick actions
- expose selection properties
- show current zoom, object count, and document state
- show file path and dirty state
- tell the application when the pointer is over UI

## Inspector

When an object is selected, the inspector can edit:

- stroke RGBA
- stroke width
- line endpoints for lines
- bounds for rectangles and ellipses

Inspector rendering is split into grouped sections:

- overview (type and selection count)
- style (stroke and width)
- geometry (line endpoints or bounds)

The inspector auto-hides when window width is constrained to preserve canvas interaction area.
When transitions are enabled in UI theme tokens, inspector show/hide uses a lightweight slide animation.

## Tooltip

Tool Rail and App Bar quick actions expose consistent hover tooltips for discoverability.

## Input Blocking

The application checks `ui_system_blocks_pointer()` before routing canvas input.

That prevents drawing or selecting through the UI.
