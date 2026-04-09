# Shape System

## Overview

The shape system provides a polymorphic, extensible architecture for different shape types using the vtable pattern.

## ShapeVTable

All shapes share a common interface defined by `ShapeVTable`:

```c
typedef struct {
    void (*destroy)(Shape*);
    void (*compute_bounds)(Shape*, float* min_x, float* min_y, float* max_x, float* max_y);
    bool (*hit_test)(Shape*, float x, float y);
    int (*get_vertex_count)(Shape*);
    void (*write_geometry)(Shape*, float* buffer);
    void (*translate)(Shape*, float dx, float dy);
    const char* (*get_property)(Shape*, const char* key);
    bool (*set_property)(Shape*, const char* key, const char* value);
} ShapeVTable;
```

## Shape Types

### LINE

- **Implementation**: `LineImpl { p1[2], p2[2] }`
- **Vertex Count**: 2
- **Primitive**: GL_LINES

### CIRCLE

- **Implementation**: `CircleImpl { center[2], radius }`
- **Vertex Count**: 64 ( tessellated)
- **Primitive**: GL_LINE_LOOP

### RECT

- **Implementation**: `RectImpl { min[2], max[2] }`
- **Vertex Count**: 4
- **Primitive**: GL_LINE_LOOP

## Shape Structure

```c
struct Shape {
    ShapeVTable* vtable;     // Function pointers
    void* impl;              // Concrete shape data
    float color[4];          // RGBA
    float line_width;        // Stroke width
    unsigned int revision;    // For cache invalidation
};
```

## Shape Registry

The registry pattern allows adding new shape types without modifying `ShapeManager` or `Renderer`.

### API

```c
// Initialize/shutdown registry
void shape_registry_init(void);
void shape_registry_shutdown(void);

// Register a shape type (called at startup)
int shape_registry_register(const char* name, ShapeCreateFunc create);

// Create a shape by name
Shape* shape_registry_create(const char* name);
```

### Registration Flow

```c
// Called once at startup
shape_registry_register("LINE", line_create);
shape_registry_register("CIRCLE", circle_create);
shape_registry_register("RECT", rect_create);
```

## Key Files

- `include/core/shape.h` - Shape struct and vtable definition
- `include/core/shape_impl.h` - Concrete implementations (LineImpl, CircleImpl, RectImpl)
- `include/core/shape_registry.h` - Registry API
- `src/shape.c` - Shape base operations
- `src/shape_registry.c` - Registry implementation

## Adding a New Shape Type

1. Define a new `*Impl` struct in `shape_impl.h`
2. Implement all `ShapeVTable` functions
3. Create a `shape_create()` function
4. Register in `shape_register_all()` in `main.c`
5. Add keyboard shortcut in `input.c`

See [Extending the Project](../extending) for full tutorial.
