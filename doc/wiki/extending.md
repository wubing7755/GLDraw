# Extending the Project

GLDraw now treats both object types and tools as manifest-registered extension
points. The registry/runtime code is separate from the app-level manifest that
assembles built-ins.

## Add a New Object Type

The usual path is to add a new `GraphicObjectDescriptor` implementation and
register it through the object manifest.

### Steps

1. Create a source file for the new object implementation.
2. Define the payload struct used by the object's `impl`.
3. Implement the descriptor callbacks you need:
   - `create`
   - `clone`
   - `destroy`
   - `translate`
   - `get_bounds`
   - `hit_test`
   - `get_path_point_count`
   - `write_path_points`
   - optional scalar property get/set
   - optional serialize/deserialize helpers
4. Register the descriptor with `register_object_type()`.
5. Add the registration function to `src/app/extension_manifest.c`.
6. Add tests for registration, serialization, and editing behavior.

### Notes

- New extension object types should normally use
  `GRAPHIC_OBJECT_INVALID` as the descriptor type so the registry can assign a
  stable runtime type value.
- Built-in compatibility constants such as `GRAPHIC_OBJECT_LINE` are kept for
  core shapes; new extensions should prefer string `type_id` lookups.
- Only extend the renderer if the object cannot be expressed through the
  existing polyline/path pipeline.

## Add a New Tool

Tools are descriptor-driven in the same way as object types.

### Steps

1. Create a source file for the tool implementation.
2. Define any tool-local state stored in `Tool.state`.
3. Implement the relevant `ToolDescriptor` callbacks:
   - `create_tool`
   - `destroy_tool`
   - optional `activate` / `deactivate`
   - `pointer_down` / `pointer_move` / `pointer_up`
   - optional `key_down`
   - optional `draw_overlay`
4. Register the descriptor with `register_tool()`, or use
   `register_shape_tool()` for standard drag-to-create shape tools.
5. Add the registration function to `src/app/tool_manifest.c`.
6. Add tests covering activation, input handling, and any command interaction.

### Notes

- Tool menu items, shortcuts, and tool-rail population are derived from
  descriptor metadata; most tools do not require additional hard-coded UI
  wiring.
- `requires_editable_layer` should be set correctly so locked-layer behavior
  stays consistent.

## Add or Change Registration Flows

The application now owns the top-level registration sequence:

```
workspace_init()
    -> app_register_all_manifests()
        -> object_manifest_register_all()
        -> tool_manifest_register_all()
```

Use these files as the primary entrypoints:

- `src/app/extension_manifest.c` for object manifests
- `src/app/tool_manifest.c` for tool manifests
- `src/app/registration_manifest.c` for the unified boot-time registration flow

The older `extension_loader_*` object APIs remain available as compatibility
wrappers, but new code should prefer the `object_manifest_*` names.

## Add Tests

Prefer adding focused tests close to the affected subsystem:

- object types: `tests/test_registry.c`, serialization tests, object-specific
  behavior tests
- tools: `tests/test_ui_logic.c`, workspace/tool interaction tests
- command integration: `tests/test_commands.c`
