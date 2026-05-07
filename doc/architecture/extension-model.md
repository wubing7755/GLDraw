# Extension Model

> Audience: contributors, maintainers
> Purpose: explain how to add object types, tools, and related registration safely
> Source of truth: `include/document/object.h`, `include/tools/tool.h`, app manifest files
> Last reviewed with code: 2026-05-07
> Related: [overview.md](overview.md), [../contributing/c-contributor-guide.zh.md](../contributing/c-contributor-guide.zh.md)

## Current Model

GLDraw treats both object types and tools as descriptor-driven extension points.

The registry/runtime code is separate from the app-level manifests that assemble built-ins.

## Add a New Object Type

Use a descriptor implementation, not a central switch expansion.

### Recommended Steps

1. Create a source file for the object implementation.
2. Define the payload struct used by `GraphicObject.impl`.
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

### Good References

- `src/document/object_line.c`
- `src/document/object_rect.c`
- `src/document/object_ellipse.c`
- `src/document/object_fake_star.c`

### Notes

- New extension object types should usually use `GRAPHIC_OBJECT_INVALID` so the registry can assign a runtime type value.
- Core compatibility constants such as `GRAPHIC_OBJECT_LINE` remain for built-ins, but new code should prefer `type_id`-based lookup and registration.
- Only extend the renderer if the object cannot be represented through the existing path/polyline rendering path.

## Add a New Tool

Tools are descriptor-driven in the same way as object types.

### Recommended Steps

1. Decide whether `register_shape_tool()` already covers the tool.
2. Create a source file for the tool implementation if a custom descriptor is needed.
3. Define any tool-local state stored in `Tool.state`.
4. Implement the relevant `ToolDescriptor` callbacks:
   - `create_tool`
   - `destroy_tool`
   - optional `activate` / `deactivate`
   - `pointer_down` / `pointer_move` / `pointer_up`
   - optional `key_down`
   - optional `draw_overlay`
5. Register the descriptor with `register_tool()` or `register_shape_tool()`.
6. Add the registration function to `src/app/tool_manifest.c`.
7. Add tests for activation, input handling, and command interaction.

### Good References

- `src/tools/tool_select.c`
- `src/tools/tool_pan.c`
- `src/tools/tool_shape.c`

### Notes

- Tool menu items and key bindings are derived from descriptor metadata and the keymap, so a new tool does not usually require large hard-coded UI changes.
- Set `requires_editable_layer` correctly so locked-layer behavior remains coherent.

## Registration Ownership

Application-owned registration sequence:

```text
workspace_init()
  -> app_register_all_manifests()
      -> object_manifest_register_all()
      -> tool_manifest_register_all()
```

Primary files:

- `src/app/extension_manifest.c`
- `src/app/tool_manifest.c`
- `src/app/registration_manifest.c`

## Command and UI Integration

When a new extension produces durable document changes:

- prefer command-based commits
- avoid leaving the final state only in overlay objects or session preview fields
- update inspector or menu actions only when the extension requires new user-facing affordances

## Testing Guidance

Prefer focused tests near the affected subsystem:

- registry and object type behavior:
  `tests/test_registry.c`
- command integration:
  `tests/test_commands.c`
- workspace and UI interaction:
  `tests/test_ui_logic.c`, `tests/test_workspace_service.c`
