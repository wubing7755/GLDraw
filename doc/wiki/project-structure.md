# Project Structure

## Directory Layout

```text
GLDraw/
├── include/
│   ├── app/
│   ├── base/
│   ├── canvas/
│   ├── document/
│   ├── platform/
│   ├── render/
│   ├── tools/
│   ├── ui/
│   ├── glad/
│   ├── KHR/
│   └── nuklear/
├── src/
│   ├── app/
│   ├── canvas/
│   ├── document/
│   ├── platform/
│   ├── render/
│   ├── tools/
│   ├── ui/
│   ├── glad.c
│   └── main.c
├── shaders/
├── doc/
└── CMakeLists.txt
```

## Module Map

| Module | Purpose |
|---|---|
| `app/` | startup, main loop, callback wiring |
| `base/` | shared math, logging, primitive types |
| `canvas/` | viewport state and world/screen conversion |
| `document/` | object model, selection, document storage |
| `platform/` | GLFW window wrapper |
| `render/` | OpenGL drawing backend |
| `tools/` | active tool routing and tool implementations |
| `ui/` | Nuklear-based editor UI |

## Important Files

| File | Role |
|---|---|
| `src/app/application.c` | application bootstrap and event bridge |
| `src/document/object.c` | object types: line, rectangle, ellipse |
| `src/document/document.c` | object list and selection set |
| `src/canvas/canvas_view.c` | zoom/pan/view transform and picking |
| `src/tools/tool_controller.c` | select, hand, line, rect, ellipse tools |
| `src/render/render_system.c` | grid and object rendering |
| `src/ui/ui_system.c` | toolbar, inspector, status bar |
