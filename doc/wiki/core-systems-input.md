# Input and Canvas Routing

## Overview

The old standalone `input.c` layer is gone. Input is now bridged in `application.c`.

## Flow

```text
GLFW callback
    -> Application callback adapter
    -> UI hit test
    -> ToolEvent creation
    -> ToolController
    -> Document / CanvasView mutation
```

## Pointer Coordinates

Each pointer event carries both:

- `screen_pos`
- `world_pos`

The world position is derived by `canvas_view_screen_to_world()`.

## Why This Matters

Mainstream canvas tools operate on document space, not raw window pixels. The current routing follows that rule:

- tools think in world coordinates
- canvas owns the transform
- window size changes do not rewrite object geometry

## Wheel Routing

Mouse wheel events are converted into zoom operations centered on the current cursor position.
