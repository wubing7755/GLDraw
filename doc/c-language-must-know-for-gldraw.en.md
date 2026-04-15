# GLDraw C Contributor Guide (Developers / Open Source Contributors)

This guide is for developers who want to read, modify, and extend GLDraw.  
It does not try to teach all of C. It focuses on the syntax and practices that matter in this repository.

Language:
- English (this file)
- 中文: [c-language-must-know-for-gldraw.md](./c-language-must-know-for-gldraw.md)

## Who this is for

- New contributors preparing to submit PRs to GLDraw
- Developers with basic C knowledge but limited project-level experience
- Contributors who need a quick map of module boundaries and change points

## Read this first before contributing

1. `doc/wiki/architecture.md`
2. `doc/wiki/data-flow.md`
3. `src/app/application.c`
4. `src/tools/tool_controller.c`
5. `src/document/object.c`

First build a clear mental model of the main path:  
`GLFW callbacks -> ToolController -> Document/CanvasView -> RenderSystem`.

## Must-know C syntax (prioritized for this project)

Every item below maps directly to real code in this repository.

| Syntax / Concept | Why it matters | Typical files |
|---|---|---|
| `struct` + `typedef` | Organizing app state and module boundaries | `include/app/workspace.h` |
| Pointers `*`, address-of `&`, member access `.` / `->` | Used across nearly all call paths | `src/app/application.c` |
| Pass-by-value vs pass-by-pointer | Avoiding copies and mutating shared state | `src/document/document.c` |
| `enum` + `switch` | Tool/object state dispatch | `include/tools/tool.h`, `src/tools/tool_controller.c` |
| Fixed arrays + bounds checks | Object pool and selection safety | `include/document/document.h` |
| Function pointers + vtable | Object/tool polymorphism | `include/document/object.h`, `src/document/object.c` |
| Callbacks + `void*` user pointer | Recover `Application*` from window callbacks | `src/app/application.c` |
| Dynamic memory management | Prevent leaks and dangling pointers | `src/document/history.c` |
| `const` / `static` / scope | Visibility and mutability control | Most `.h/.c` files |
| Preprocessor platform branches | Windows and non-Windows behavior | `src/document/persistence.c` |
| Bitmask operations | Modifier key handling | `src/tools/tool_controller.c` |
| String formatting safety | Status text, paths, logging | `src/app/application.c` |

## Key syntax patterns used in GLDraw

### 1) Callback bridge (must understand)

```c
glfwSetWindowUserPointer(app->window.handle, app);
Application* app = (Application*)glfwGetWindowUserPointer(handle);
```

Why: GLFW callbacks only provide `GLFWwindow*`.  
The user pointer is how callbacks recover the application state for that window.

### 2) Vtable-style polymorphism

```c
struct GraphicObjectVTable {
    void (*translate)(GraphicObject* object, Vec2 delta);
    int (*hit_test)(const GraphicObject* object, Vec2 point, float tolerance);
};
```

Why: New object types can be added by implementing a function set and wiring a vtable, not by scattering `if/else`.

### 3) Consistent null-check guard

```c
if (!app) {
    return;
}
```

Why: Reduces crash risk and matches the repository coding style.

## Contributor change entry points (by task type)

### Add a new shape/object type

1. Add `GraphicObjectType` in `include/document/object.h`
2. Implement constructor + vtable callbacks in `src/document/object.c`
3. Ensure hit test, path points, scalar get/set are complete
4. If needed, add tool creation flow and inspector bindings

### Add a new tool

1. Add `ToolKind` in `include/tools/tool.h`
2. Add state struct + callbacks in `src/tools/tool_controller.c`
3. Register tool in `tool_controller_init()`
4. Add toolbar button and shortcut mapping

### Change inspector/property editing behavior

1. Edit `src/ui/ui_system.c`
2. Ensure object edits correctly push to `document_history_push()`
3. Verify dirty state sync via `workspace_sync_document_dirty()`

## Pre-PR checklist

1. The app builds and runs locally
2. Manual flow check for create/select/move/undo/redo
3. No obvious leaks (allocation/free ownership is paired)
4. No array bound breakage or missing null checks
5. Docs/comments are updated when behavior changes

## Common issues for new contributors

### Issue 1: Mixing `.` and `->`

- Struct variable: `.`
- Pointer: `->`

### Issue 2: Object changes not recorded in history

Symptom: `Ctrl+Z` does not undo your edit.  
Check: Did your code call `document_history_push()` at the right place?

### Issue 3: Rendering changed, model unchanged

Symptom: UI looks right, but save/load loses data.  
Check: Did you update `Document` and persistence logic, not just renderer output?

### Issue 4: Missing platform branch handling

Symptom: Works on Windows but fails on Linux/macOS (or vice versa).  
Check: Is your `#ifdef _WIN32` split complete?

## Suggested learning and contribution path

1. Read-only walkthrough: main loop, event routing, render path
2. Small edits: shortcut, status text, tool switching behavior
3. Medium edits: add/modify object properties, tweak tool behavior
4. Structural edits: new shape type, history extension, persistence format changes

## Minimum bar for independent contribution

You are ready for small-to-medium PRs when you can:

1. Trace `Application*` through callback routing
2. Modify `Document` data safely with bounds/guard checks
3. Implement or extend tool/object callbacks (vtable style)
4. Find and fix one memory ownership issue
5. Complete a full local change-build-run verification loop
