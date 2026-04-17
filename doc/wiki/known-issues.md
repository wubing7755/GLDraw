# Known Issues

## Rendering

- `glLineWidth()` is driver-dependent, so thick strokes are approximate.
- All objects are outline-only; there is no fill pipeline yet.
- Object geometry is still rebuilt on the CPU every frame.
- Grid and object strokes are frame-batched, but there is still no retained GPU geometry cache.

## Document Model

- The document still uses a fixed-size object array.
- Undo/redo is mixed mode: snapshots plus lightweight scalar/translate transactions.
- Persistence is JSON-only (now with stricter load-time validation/logging, but no schema version migration yet).
- There are no layers, groups, or visibility/lock flags yet.

## UI

- The UI is functional but still minimal.
- Menus exist, but modal dialogs and dockable panels are still missing.
- Property editing only targets the primary selected object.

## Editor Features

- No snapping, guides, or rulers
- No box selection
- No PNG export
- No cut/copy/paste workflow
- Theme hot-reload keeps last valid runtime theme set, but malformed theme files are ignored until fixed
