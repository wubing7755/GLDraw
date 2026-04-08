# Known Issues / Limitations

## Current Limitations

### Rendering

- **Line width is not actual pixel width** - OpenGL's `glLineWidth()` is a hint and may not match actual pixels. For precise control, consider geometry-based thick lines.

- **No fill rendering** - All shapes render as outlines (GL_LINES/GL_LINE_LOOP). Fill support would require separate code paths.

- **No anti-aliasing** - Shapes are rendered without MSAA or other smoothing techniques.

### Shape System

- **Max 256 shapes** - ShapeManager uses a fixed-size array. Larger projects would need dynamic allocation.

- **No shape IDs** - Selection uses pointer-based tracking. Shapes cannot be referenced by stable IDs.

- **No undo/redo** - Operations are immediately applied. The `Ctrl+Z` shortcut only deletes the last shape.

### UI

- **Fixed property panel position** - The UI is hardcoded at x=580, y=50. Resizing is not supported.

- **No zoom/pan controls** - The viewport is fixed.

- **Nuklear only supports property panel** - No menus, toolbars, or dialogs.

### Input

- **Limited keyboard shortcuts** - Only basic tool switching and delete last shape are implemented.

- **No gamepad/controller support** - Input is keyboard and mouse only.

### Performance

- **Per-shape draw calls** - Each shape calls `glDrawArrays()` separately. Batching would improve performance for many shapes.

- **No spatial indexing** - Hit testing iterates all shapes. Spatial partitioning (quadtree) would improve selection performance.

## Assumptions

These may change in future versions:

- OpenGL 3.3 is the minimum version
- Default window size is 800×600 (configurable via `WINDOW_WIDTH`/`WINDOW_HEIGHT` in `include/core/window.h`)
- Shader files must be in `shaders/` directory relative to working directory
