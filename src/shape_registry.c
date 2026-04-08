#include <stdio.h>
#include <string.h>
#include <core/shape_registry.h>
#include <core/macros.h>

/* =============================================================================
 * Internal registry table — fixed size array of registered shape types
 * Phase 2 supports up to 8 shape types
 * =============================================================================
 */

#define MAX_SHAPE_TYPES 8

typedef struct {
    const char* name;
    ShapeCreateFn create_fn;
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

void shape_register(const char* type_name, ShapeCreateFn create_fn, ShapeVTable* vtable)
{
    if (UNLIKELY(s_registry_count >= MAX_SHAPE_TYPES)) {
        LOG_ERROR_F("Max shape types reached (%d)", MAX_SHAPE_TYPES);
        return;
    }

    if (UNLIKELY(!type_name || !create_fn || !vtable)) {
        LOG_ERROR("Invalid registration for shape type");
        return;
    }

    /* Check for duplicate registration */
    for (int i = 0; i < s_registry_count; i++) {
        if (strcmp(s_registry[i].name, type_name) == 0) {
            LOG_WARN_F("'%s' already registered", type_name);
            return;
        }
    }

    s_registry[s_registry_count].name = type_name;
    s_registry[s_registry_count].create_fn = create_fn;
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

Shape* shape_registry_create(const char* type_name,
                             float r, float g, float b, float a,
                             float line_width)
{
    for (int i = 0; i < s_registry_count; i++) {
        if (strcmp(s_registry[i].name, type_name) == 0) {
            return s_registry[i].create_fn(r, g, b, a, line_width);
        }
    }
    return NULL;
}
