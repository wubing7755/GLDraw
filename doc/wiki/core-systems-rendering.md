# Rendering System

## Overview

`RenderSystem` is an OpenGL line renderer that consumes:

- the active `Document`
- the active `CanvasView`
- the active tool overlay object

It does not own editing state.

## Render Order

1. clear using the canvas background color
2. draw grid lines and origin axes
3. draw document objects
4. draw selection highlight
5. draw current tool overlay
6. let Nuklear render on top

## Geometry Model

Each object exposes:

- `get_path_point_count`
- `write_path_points`

The renderer asks the object for a world-space polyline, converts it through `CanvasView`, expands strip segments when needed, appends into a frame batch, then flushes compatible draws together.

## Current Object Rendering

- `Line`: 2 points
- `Rectangle`: 5 points, closed loop
- `Ellipse`: 65 points, closed loop approximation

Batching behavior:

- grid + axes + object strokes share a frame-level line batch when primitive/line-width are compatible
- object `GL_LINE_STRIP` paths are normalized into segment pairs for safe cross-object batching
- flush points happen only when state is incompatible or at frame end

## Shader

The shader now works in screen-pixel coordinates. The vertex shader converts screen-space positions into NDC using `uScreenSize`.

That keeps the projection logic simple:

- world -> screen is done in C through `CanvasView`
- screen -> NDC is done in GLSL

## Limits

- line width still relies on `glLineWidth`
- rendering is outline-only
- there is no retained GPU cache yet (object paths are rebuilt on CPU each frame)
