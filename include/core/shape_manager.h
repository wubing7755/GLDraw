#ifndef SHAPE_MANAGER_H
#define SHAPE_MANAGER_H

#include "shape.h"

#define SHAPE_MAX_LINES 256

/* =============================================================================
 * Phase 1: Minimal ShapeManager — dynamic array, no registry
 * =============================================================================
 *
 * Interface intentionally simple: add / remove_last / foreach.
 * Phase 2 will add hit_test / selection after vtable introduction.
 * =============================================================================
 */

void sm_init(void);
void sm_shutdown(void);

void sm_add(Shape* s);
void sm_remove_last(void);

int sm_count(void);
Shape* sm_get(int index);

#endif /* SHAPE_MANAGER_H */
