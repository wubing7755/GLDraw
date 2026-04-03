#ifndef SHAPE_REGISTRY_H
#define SHAPE_REGISTRY_H

#include "shape.h"

/* =============================================================================
 * Phase 2: Shape Registry — single registration point for all shape types
 *
 * Each shape type registers its vtable via shape_register().
 * New shapes are added WITHOUT modifying ShapeManager or Renderer.
 * =============================================================================
 */

void shape_registry_init(void);
void shape_registry_shutdown(void);

/* Register a shape type — called once at startup per type */
void shape_register(const char* type_name, ShapeVTable* vtable);

/* Get vtable by type name — returns NULL if not found */
ShapeVTable* shape_registry_get(const char* type_name);

#endif /* SHAPE_REGISTRY_H */
