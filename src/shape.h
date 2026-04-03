#ifndef SHAPE_H
#define SHAPE_H

#include <stddef.h>

/* =============================================================================
 * Phase 1: Minimal LINE only — no vtable, no registry, no tool abstraction
 * =============================================================================
 *
 * Why: Verify "data → render" pipeline first. Any premature abstraction
 *       (vtable/registry) before the core loop works is a liability.
 *
 * This is intentionally simple: LineData embedded directly.
 * Phase 2 will introduce vtable + registry when we add CIRCLE/RECTANGLE.
 * =============================================================================
 */

#define SHAPE_MAX_LINES 256

/* Line-specific geometry data — Phase 1 only has LINE */
typedef struct {
    float p1[2];    /* Start point (x, y) */
    float p2[2];    /* End point (x, y) */
} LineData;

/* Shape — Phase 1 only holds LineData, no vtable/impl */
typedef struct {
    LineData line;
    float color[4];    /* RGBA */
    float line_width;
} Shape;

/* Shape lifecycle */
Shape* shape_create_line(float x1, float y1, float x2, float y2,
                        float r, float g, float b, float a,
                        float line_width);
void shape_destroy(Shape* s);

#endif /* SHAPE_H */
