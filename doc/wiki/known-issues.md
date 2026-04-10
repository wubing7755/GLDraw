# Known Issues

## Rendering

- `glLineWidth()` is driver-dependent, so thick strokes are approximate.
- All objects are outline-only; there is no fill pipeline yet.
- The renderer uploads transient geometry every frame instead of caching draw data.

## Document Model

- The document still uses a fixed-size object array.
- Undo/redo currently uses full document snapshots, not compact commands.
- There is still no persistence layer.
- There are no layers, groups, or visibility/lock flags yet.

## UI

- The UI is functional but still minimal.
- There are no dialogs, menus, or dockable panels.
- Property editing only targets the primary selected object.

## Editor Features

- No snapping, guides, or rulers
- No box selection
- No frame-all / zoom-to-fit command
- No save/load
