# Tool System

## Overview

`ToolController` owns one instance of each tool and tracks which one is active.

Current tools:

- Select
- Hand
- Line
- Rectangle
- Ellipse

## Tool Interface

Each tool may implement:

- activate
- deactivate
- pointer_down
- pointer_move
- pointer_up
- key_down

Every callback receives a `ToolContext` containing the workspace, document, and canvas.

## Interaction Model

### Select Tool

- picks objects using the canvas
- updates the document selection set
- commits selection-only changes into history
- drags selected objects in world space

### Hand Tool

- pans the canvas using screen-space delta

### Shape Tools

- capture an anchor point on pointer down
- update a temporary overlay object while dragging
- create a document object on pointer up

## Keyboard Routing

- `V` -> Select
- `H` -> Hand
- `L` -> Line
- `R` -> Rectangle
- `E` -> Ellipse
- `Delete` / `Backspace` -> delete selection
- `Ctrl+Z` / `Ctrl+Y` / `Ctrl+Shift+Z` -> history navigation
