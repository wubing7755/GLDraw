# Rendering System

## Overview

The renderer manages OpenGL VAO/VBO for shape geometry and handles all drawing operations.

## Vertex Format

Shapes are rendered with per-vertex color using the following format:

| Offset | Type | Description |
|--------|------|-------------|
| 0 | float | X position |
| 4 | float | Y position |
| 8 | float | R (red) |
| 12 | float | G (green) |
| 16 | float | B (blue) |
| 20 | float | A (alpha) |

**Stride**: 24 bytes (6 floats × 4 bytes)

## Vertex Buffer

- **Dynamic VBO** - Updated when shapes change
- **Dirty tracking** - Avoids unnecessary GPU uploads by tracking shape revisions
- **Per-shape draw calls** - Each shape calls `glDrawArrays` separately

## Rendering Pipeline

```
ShapeManager → Shape → vtable.write_geometry()
                            ↓
                      Vertex Buffer (VBO)
                            ↓
                      glDrawArrays()
```

## Primitive Types

| Shape Type | Primitive | Vertex Count |
|------------|-----------|---------------|
| LINE | GL_LINES | 2 |
| CIRCLE | GL_LINE_LOOP | 64 |
| RECT | GL_LINE_LOOP | 4 |

## Key Files

- `src/renderer.c` - VAO/VBO setup, render loop
- `src/shader.c` - Shader compilation and linking
- `include/core/renderer.h` - Public API
- `include/core/shader.h` - Shader API

## API

```c
// Initialize renderer with shader program
void init_renderer(GLuint shader_program);

// Mark renderer as needing update (called by shape changes)
void renderer_mark_dirty(void);

// Render all shapes from ShapeManager
void render_shapes(ShapeManager* sm, SelectionManager* sel);
```

## Selection Highlighting

When a shape is selected, the renderer draws it with a yellow outline instead of its normal color. This is handled by checking `sel_contains()` during the render loop.

## OpenGL State

The renderer enables:
- `GL_BLEND` - For transparency
- `GL_LINE_LOOP` / `GL_LINES` - Shape primitives
- `GL_PROGRAM_POINT_SIZE` - For future point rendering

## Shader Details

### Vertex Shader (`shaders/basic.vert`)

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
out vec4 vColor;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
```

### Fragment Shader (`shaders/basic.frag`)

```glsl
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
```
