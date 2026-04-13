# UI System

## Overview

`UiSystem` uses Nuklear to build editor chrome around the canvas.

Current panels:

- top toolbar
- right inspector
- bottom status bar

## Responsibilities

- switch the active tool
- expose save / load document commands
- expose selection properties
- show current zoom, object count, and view center
- show file path and dirty state
- tell the application when the pointer is over UI

## Inspector

When an object is selected, the inspector can edit:

- stroke RGBA
- stroke width
- line endpoints for lines
- bounds for rectangles and ellipses

## Input Blocking

The application checks `ui_system_blocks_pointer()` before routing canvas input.

That prevents drawing or selecting through the UI.
