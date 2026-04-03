#ifndef SHAPE_GEOMETRY_H
#define SHAPE_GEOMETRY_H

#include "shape.h"

/* =============================================================================
 * Phase 2: Geometry utilities — hit_test and bounds calculation
 *
 * These are pure math functions, no OpenGL or platform dependencies.
 * =============================================================================
 */

/* Compute axis-aligned bounding box */
void shape_geometry_bounds(const Shape* s, float* min_x, float* min_y,
                           float* max_x, float* max_y);

/* Hit test: point vs shape (with tolerance) */
int shape_geometry_hit_test(const Shape* s, float x, float y, float tolerance);

#endif /* SHAPE_GEOMETRY_H */
