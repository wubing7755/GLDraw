#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shape.h"

/* =============================================================================
 * Phase 1: LINE only implementation
 * =============================================================================
 */

Shape* shape_create_line(float x1, float y1, float x2, float y2,
                        float r, float g, float b, float a,
                        float line_width)
{
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (!s) {
        fprintf(stderr, "[Shape] Failed to allocate shape\n");
        return NULL;
    }

    s->line.p1[0] = x1;
    s->line.p1[1] = y1;
    s->line.p2[0] = x2;
    s->line.p2[1] = y2;
    s->color[0] = r;
    s->color[1] = g;
    s->color[2] = b;
    s->color[3] = a;
    s->line_width = line_width;

    printf("[Shape] Created LINE: (%.2f,%.2f) → (%.2f,%.2f)\n",
           x1, y1, x2, y2);
    return s;
}

void shape_destroy(Shape* s)
{
    if (s) {
        printf("[Shape] Destroyed LINE\n");
        free(s);
    }
}
