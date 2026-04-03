#include <stdio.h>
#include <string.h>
#include <core/shape_registry.h>

/* =============================================================================
 * Internal registry table — fixed size array of registered shape types
 * Phase 2 supports up to 8 shape types
 * =============================================================================
 */

#define MAX_SHAPE_TYPES 8

typedef struct {
    const char* name;
    ShapeVTable* vtable;
    int used;
} RegistryEntry;

static RegistryEntry s_registry[MAX_SHAPE_TYPES];
static int s_registry_count = 0;

void shape_registry_init(void)
{
    s_registry_count = 0;
    for (int i = 0; i < MAX_SHAPE_TYPES; i++) {
        s_registry[i].used = 0;
    }
}

void shape_registry_shutdown(void)
{
    /* Nothing to clean up — vtables are statically allocated */
}

void shape_register(const char* type_name, ShapeVTable* vtable)
{
    if (s_registry_count >= MAX_SHAPE_TYPES) {
        fprintf(stderr, "[Registry] ERROR: max shape types reached (%d)\n", MAX_SHAPE_TYPES);
        return;
    }

    /* Check for duplicate registration */
    for (int i = 0; i < s_registry_count; i++) {
        if (strcmp(s_registry[i].name, type_name) == 0) {
            fprintf(stderr, "[Registry] WARNING: '%s' already registered\n", type_name);
            return;
        }
    }

    s_registry[s_registry_count].name = type_name;
    s_registry[s_registry_count].vtable = vtable;
    s_registry[s_registry_count].used = 1;
    s_registry_count++;
}

ShapeVTable* shape_registry_get(const char* type_name)
{
    for (int i = 0; i < s_registry_count; i++) {
        if (strcmp(s_registry[i].name, type_name) == 0) {
            return s_registry[i].vtable;
        }
    }
    return NULL;
}
