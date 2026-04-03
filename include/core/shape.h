#ifndef SHAPE_H
#define SHAPE_H

#include <stddef.h>

/* =============================================================================
 * Phase 2: Shape with vtable + impl decoupling
 *
 * Key changes from Phase 1:
 * - Shape now has vtable pointer (function table for polymorphic behavior)
 * - Geometry data stored in void* impl (concrete type per shape)
 * - Registry provides single registration point for new shapes
 * =============================================================================
 */

typedef struct Shape Shape;
typedef struct ShapeVTable ShapeVTable;

/* =============================================================================
 * ShapeVTable — function pointers for polymorphic behavior
 * =============================================================================
 */
struct ShapeVTable {
    const char* name;
    void (*destroy)(Shape*);
    void (*compute_bounds)(const Shape*, float* minX, float* minY, float* maxX, float* maxY);
    int (*hit_test)(const Shape*, float x, float y, float tolerance);
    void (*get_geometry)(const Shape*, float** out_vertices, int* out_count);
};

/* =============================================================================
 * Shape — polymorphic container
 * =============================================================================
 */
struct Shape {
    ShapeVTable* vtable;   /* Polymorphic function table */
    void* impl;            /* Pointer to concrete shape data (LineImpl*, CircleImpl*, etc.) */
    float color[4];        /* RGBA */
    float line_width;
};

/* =============================================================================
 * Shape lifecycle
 * =============================================================================
 */

/* Create shape by type name — uses registry to find constructor */
Shape* shape_create(const char* type_name,
                    float r, float g, float b, float a,
                    float line_width);

void shape_destroy(Shape* s);

/* Register all built-in shape types (called once at init) */
void shape_register_all(void);

/* =============================================================================
 * Shape operations — delegated to vtable
 * =============================================================================
 */
void shape_get_bounds(const Shape* s, float* minX, float* minY, float* maxX, float* maxY);
int shape_hit_test(const Shape* s, float x, float y, float tolerance);
void shape_get_geometry(const Shape* s, float** out_vertices, int* out_count);

#endif /* SHAPE_H */
