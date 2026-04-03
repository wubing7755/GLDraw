#include <math.h>
#include "shape_geometry.h"
#include "shape_impl.h"  /* for concrete impl types */

/* =============================================================================
 * Geometry implementations per shape type
 * =============================================================================
 */

static void bounds_line(const Shape* s, float* min_x, float* min_y,
                        float* max_x, float* max_y)
{
    LineImpl* line = (LineImpl*)s->impl;
    float x1 = line->p1[0], y1 = line->p1[1];
    float x2 = line->p2[0], y2 = line->p2[1];
    *min_x = (x1 < x2) ? x1 : x2;
    *min_y = (y1 < y2) ? y1 : y2;
    *max_x = (x1 > x2) ? x1 : x2;
    *max_y = (y1 > y2) ? y1 : y2;
}

static void bounds_circle(const Shape* s, float* min_x, float* min_y,
                          float* max_x, float* max_y)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    *min_x = c->center[0] - c->radius;
    *min_y = c->center[1] - c->radius;
    *max_x = c->center[0] + c->radius;
    *max_y = c->center[1] + c->radius;
}

static void bounds_rect(const Shape* s, float* min_x, float* min_y,
                       float* max_x, float* max_y)
{
    RectImpl* r = (RectImpl*)s->impl;
    float x1 = r->min[0], y1 = r->min[1];
    float x2 = r->max[0], y2 = r->max[1];
    *min_x = (x1 < x2) ? x1 : x2;
    *min_y = (y1 < y2) ? y1 : y2;
    *max_x = (x1 > x2) ? x1 : x2;
    *max_y = (y1 > y2) ? y1 : y2;
}

/* Distance from point to line segment */
static float distance_point_to_line(float px, float py,
                                   float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len_sq = dx*dx + dy*dy;

    if (len_sq < 1e-6f) {
        /* Degenerate line — return distance to endpoint */
        float ddx = px - x1;
        float ddy = py - y1;
        return sqrtf(ddx*ddx + ddy*ddy);
    }

    /* Project point onto line */
    float t = ((px - x1)*dx + (py - y1)*dy) / len_sq;
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;

    float proj_x = x1 + t * dx;
    float proj_y = y1 + t * dy;
    float ddx = px - proj_x;
    float ddy = py - proj_y;
    return sqrtf(ddx*ddx + ddy*ddy);
}

static int hit_test_line(const Shape* s, float x, float y, float tolerance)
{
    LineImpl* line = (LineImpl*)s->impl;
    float dist = distance_point_to_line(x, y,
                                        line->p1[0], line->p1[1],
                                        line->p2[0], line->p2[1]);
    return (dist <= tolerance) ? 1 : 0;
}

static int hit_test_circle(const Shape* s, float x, float y, float tolerance)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    float dx = x - c->center[0];
    float dy = y - c->center[1];
    float dist = sqrtf(dx*dx + dy*dy);
    /* Hit if within tolerance of the circumference */
    return (fabsf(dist - c->radius) <= tolerance) ? 1 : 0;
}

static int hit_test_rect(const Shape* s, float x, float y, float tolerance)
{
    RectImpl* r = (RectImpl*)s->impl;
    float x1 = r->min[0], y1 = r->min[1];
    float x2 = r->max[0], y2 = r->max[1];

    /* Expand bounds by tolerance and check if point is within */
    float min_x = (x1 < x2) ? x1 : x2;
    float min_y = (y1 < y2) ? y1 : y2;
    float max_x = (x1 > x2) ? x1 : x2;
    float max_y = (y1 > y2) ? y1 : y2;

    return (x >= min_x - tolerance && x <= max_x + tolerance &&
            y >= min_y - tolerance && y <= max_y + tolerance) ? 1 : 0;
}

/* =============================================================================
 * Public API — dispatches to concrete type
 * =============================================================================
 */

void shape_geometry_bounds(const Shape* s, float* min_x, float* min_y,
                           float* max_x, float* max_y)
{
    if (!s || !s->vtable || !s->vtable->compute_bounds) {
        *min_x = *min_y = *max_x = *max_y = 0.0f;
        return;
    }
    s->vtable->compute_bounds(s, min_x, min_y, max_x, max_y);
}

int shape_geometry_hit_test(const Shape* s, float x, float y, float tolerance)
{
    if (!s || !s->vtable || !s->vtable->hit_test) {
        return 0;
    }
    return s->vtable->hit_test(s, x, y, tolerance);
}
