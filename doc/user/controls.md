# Controls

> Audience: users, contributors
> Purpose: document default tool shortcuts and editor actions
> Source of truth: `src/input/keymap.c`, `src/tools/tool_*.c`, `src/ui/ui_menu_def.c`
> Last reviewed with code: 2026-05-07
> Related: [getting-started.md](getting-started.md)

## Tool Shortcuts

| Tool | Default shortcut | Source |
|---|---|---|
| Select | `V` | `src/tools/tool_select.c` |
| Hand / Pan | `H` | `src/tools/tool_pan.c` |
| Line | `L` | `src/tools/tool_shape.c` |
| Rectangle | `R` | `src/tools/tool_shape.c` |
| Ellipse | `E` | `src/tools/tool_shape.c` |

## File Shortcuts

| Action | Default shortcut |
|---|---|
| New document | `Ctrl+N` |
| Open | `Ctrl+O` |
| Save | `Ctrl+S` |

## Edit Shortcuts

| Action | Default shortcut |
|---|---|
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` or `Ctrl+Shift+Z` |
| Cut | `Ctrl+X` |
| Copy | `Ctrl+C` |
| Paste | `Ctrl+V` |
| Select all | `Ctrl+A` |

## View Shortcuts

| Action | Default shortcut |
|---|---|
| Zoom in | `Ctrl+Plus` |
| Zoom out | `Ctrl+Minus` |
| Zoom to fit | `Ctrl+0` |

## Pointer and Selection Behavior

- Shape tools create objects by left-dragging on the canvas.
- The select tool uses click-to-select and `Shift+Click` to toggle selection membership.
- Dragging an active selection previews movement during the drag and commits a move command on release.
- The hand tool pans the canvas with left-drag.
- The mouse wheel zooms at the cursor.

## Notes for Maintainers

- Tool defaults come from `ToolDescriptor.default_shortcut`.
- Global action defaults come from the keymap table in `src/input/keymap.c`.
- Menus may render display variants such as `Ctrl++` for `Ctrl+Plus`; update both the keymap and menu copy when changing user-visible bindings.
