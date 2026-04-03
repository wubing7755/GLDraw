#ifndef SHAPE_IMPL_H
#define SHAPE_IMPL_H

#include <stddef.h>

/* =============================================================================
 * Phase 2: Concrete shape data structures — stored in Shape::impl
 *
 * These are NOT visible to ShapeManager or Renderer — only to vtable functions.
 * =============================================================================
 */

/* Line: two endpoints */
typedef struct {
    float p1[2];   /* Start point */
    float p2[2];   /* End point */
} LineImpl;

/* Circle: center + radius */
typedef struct {
    float center[2];
    float radius;
} CircleImpl;

/* Rectangle: min/max corners */
typedef struct {
    float min[2];  /* Bottom-left */
    float max[2];  /* Top-right */
} RectImpl;

#endif /* SHAPE_IMPL_H */
