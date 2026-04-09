# Extending the Project

## Adding a New Shape Type

This tutorial adds a **Triangle** shape.

### Step 1: Define the Implementation

In `include/core/shape_impl.h`, add:

```c
typedef struct {
    float p1[2], p2[2], p3[2];  // Three vertices
} TriangleImpl;
```

### Step 2: Implement ShapeVTable Functions

In a new file `src/shape_triangle.c`:

```c
static void triangle_destroy(Shape* shape) {
    free(shape->impl);
    free(shape);
}

static void triangle_compute_bounds(Shape* shape, float* min_x, float* min_y, float* max_x, float* max_y) {
    TriangleImpl* tri = (TriangleImpl*)shape->impl;
    // Compute bounding box from three vertices
}

static bool triangle_hit_test(Shape* shape, float x, float y) {
    // Point-in-triangle test
}

static int triangle_get_vertex_count(Shape* shape) {
    return 3;
}

static void triangle_write_geometry(Shape* shape, float* buffer) {
    TriangleImpl* tri = (TriangleImpl*)shape->impl;
    // Write 3 vertices with color to buffer
}

static void triangle_translate(Shape* shape, float dx, float dy) {
    TriangleImpl* tri = (TriangleImpl*)shape->impl;
    // Translate all three vertices
}

Shape* triangle_create(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y) {
    // Allocate and initialize shape
}
```

### Step 3: Register the Shape

In `src/shape_registry.c`, add to `shape_register_all()`:

```c
shape_registry_register("TRIANGLE", triangle_create);
```

In `include/core/shape.h`, add to `ShapeType` enum:

```c
typedef enum { SHAPE_LINE, SHAPE_CIRCLE, SHAPE_RECT, SHAPE_TRIANGLE } ShapeType;
```

### Step 4: Add Keyboard Shortcut

In `src/input.c`, `key_callback()`:

```c
case GLFW_KEY_4:
    toolmanager_set_tool(draw_tool_create("TRIANGLE"));
    break;
```

### Step 5: Update CMakeLists.txt

In `CMakeLists.txt`, find the `add_executable(GLDraw ...` line and add your new source file:

```cmake
add_executable(GLDraw
    src/main.c
    src/shape.c
    src/shape_triangle.c    # <-- Add this line
    # ... other sources
)
```

## Adding a New Tool

### Step 1: Define Tool Context

```c
typedef struct {
    // Tool-specific state
} MyToolCtx;
```

### Step 2: Implement ToolVTable

```c
static void mytool_on_down(Tool* tool, float x, float y, SelectionManager* sel, int shift) {
    MyToolCtx* ctx = (MyToolCtx*)tool->ctx;
    // Handle mouse down
}

static void mytool_on_move(Tool* tool, float x, float y, SelectionManager* sel) {
    // Handle mouse move
}

static void mytool_on_up(Tool* tool, SelectionManager* sel) {
    // Handle mouse up
}
```

### Step 3: Create Tool Factory

```c
Tool* mytool_create(void) {
    Tool* tool = tool_create(sizeof(MyToolCtx));
    tool->vtable->name = mytool_name;
    tool->vtable->on_down = mytool_on_down;
    tool->vtable->on_move = mytool_on_move;
    tool->vtable->on_up = mytool_on_up;
    return tool;
}
```

## Hooking into the Render Loop

To add custom rendering (e.g., grid, guides):

1. Modify `render_frame()` in `main.c`
2. Add your rendering code before or after `render_shapes()`

```c
render_grid();      // Your custom rendering
render_shapes();    // Built-in shape rendering
render_overlay();   // Your custom overlay
```
