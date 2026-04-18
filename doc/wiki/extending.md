# Extending the Project

## Add a New Object Type

The simplest extension path is to add another `GraphicObject` implementation.

### Steps

1. Add a new `GraphicObjectType` in `include/document/object.h`.
2. Define a concrete payload struct in `src/document/object.c`, or split object types into separate files if the module grows.
3. Implement the vtable functions:
   - destroy
   - translate
   - bounds
   - hit test
   - path writing
   - scalar property get/set
4. Add a constructor like `object_create_triangle(...)`.
5. Extend the renderer only if the object cannot be expressed as a polyline.
6. Add a tool or inspector bindings if the object needs creation or editing UI.

## Add a New Tool

### Steps

1. Add a new `ToolKind` in `include/tools/tool.h`.
2. Define a tool state struct in `src/tools/tool_controller.c`.
3. Implement the relevant callbacks.
4. Register the tool in `tool_controller_init()`.
5. Add a toolbar button and optional keyboard shortcut.

## Add Layers

To add layer support:

1. Add layer containers inside `Document`
2. Move object storage from one flat array to per-layer lists
3. Keep `CanvasView` unchanged
4. Keep tools operating on active layer selection

See `include/document/document.h` for the current flat structure as a starting point.
