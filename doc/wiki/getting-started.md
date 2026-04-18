# Getting Started

## Build

### Linux / macOS

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/GLDraw
```

Or use the convenience script:

```sh
./build.sh          # Release build
./build.sh debug    # Debug build
./build.sh clean    # Clean artifacts
```

For other platforms, see the main [README.md](../../README.md).

## First Session

1. Start the application.
2. Press `L`, `R`, or `E` to choose a drawing tool.
3. Drag on the canvas to create an object.
4. Press `V` to switch to the select tool.
5. Click to select, `Shift+Click` to multi-select.
6. Drag selected objects to move them.
7. Use the inspector (right panel) to edit stroke color, width, and geometry.
8. Use the mouse wheel to zoom at the cursor.
9. Press `Ctrl+S` to save, `Ctrl+O` to load.

## Runtime Notes

- The canvas stores world-space data independently from the window size.
- Document persistence saves to `document.json` or the current path.
- Theme selection is persisted in `gldraw.settings.json`.
