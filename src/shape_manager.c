#include <stdio.h>
#include <stdlib.h>
#include <core/shape_manager.h>
#include <core/macros.h>

/* =============================================================================
 * Phase 1: Dynamic array implementation
 * =============================================================================
 */

static Shape* s_shapes[SHAPE_MAX_LINES];
static int s_count = 0;
static unsigned int s_revision = 0;

void sm_init(void)
{
    s_count = 0;
    s_revision = 1;
}

void sm_shutdown(void)
{
    for (int i = 0; i < s_count; i++) {
        shape_destroy(s_shapes[i]);
        s_shapes[i] = NULL;
    }
    s_count = 0;
    s_revision++;
}

int sm_add(Shape* s)
{
    if (UNLIKELY(!s || s_count >= SHAPE_MAX_LINES)) {
        return 0;
    }
    s_shapes[s_count++] = s;
    s_revision++;
    return 1;
}

Shape* sm_take_last(void)
{
    Shape* shape = NULL;

    if (UNLIKELY(s_count <= 0)) {
        return NULL;
    }

    s_count--;
    shape = s_shapes[s_count];
    s_shapes[s_count] = NULL;
    s_revision++;
    return shape;
}

void sm_remove_last(void)
{
    Shape* shape = sm_take_last();
    if (shape) {
        shape_destroy(shape);
    }
}

int sm_count(void)
{
    return s_count;
}

Shape* sm_get(int index)
{
    if (UNLIKELY(index < 0 || index >= s_count)) {
        return NULL;
    }
    return s_shapes[index];
}

unsigned int sm_get_revision(void)
{
    return s_revision;
}
