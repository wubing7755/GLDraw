#include <stdio.h>
#include <stdlib.h>
#include "shape_manager.h"

/* =============================================================================
 * Phase 1: Dynamic array implementation
 * =============================================================================
 */

static Shape* s_shapes[SHAPE_MAX_LINES];
static int s_count = 0;

void sm_init(void)
{
    s_count = 0;
}

void sm_shutdown(void)
{
    for (int i = 0; i < s_count; i++) {
        shape_destroy(s_shapes[i]);
        s_shapes[i] = NULL;
    }
    s_count = 0;
}

void sm_add(Shape* s)
{
    if (s_count >= SHAPE_MAX_LINES) {
        return;
    }
    s_shapes[s_count++] = s;
}

void sm_remove_last(void)
{
    if (s_count <= 0) {
        return;
    }
    s_count--;
    shape_destroy(s_shapes[s_count]);
    s_shapes[s_count] = NULL;
}

int sm_count(void)
{
    return s_count;
}

Shape* sm_get(int index)
{
    if (index < 0 || index >= s_count) {
        return NULL;
    }
    return s_shapes[index];
}
