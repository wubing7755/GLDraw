#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <core/shape.h>
#include <core/shape_impl.h>
#include <core/shape_registry.h>
#include <core/shape_geometry.h>

/* =============================================================================
 * Concrete vtable implementations
 * =============================================================================
 */

/* ---------- LINE ---------- */
static void line_destroy(Shape* s)
{
    if (!s) return;
    free(s->impl);
    free(s);
}

static void line_bounds(const Shape* s, float* minX, float* minY, float* maxX, float* maxY)
{
    LineImpl* line = (LineImpl*)s->impl;
    float x1 = line->p1[0], y1 = line->p1[1];
    float x2 = line->p2[0], y2 = line->p2[1];
    *minX = (x1 < x2) ? x1 : x2;
    *minY = (y1 < y2) ? y1 : y2;
    *maxX = (x1 > x2) ? x1 : x2;
    *maxY = (y1 > y2) ? y1 : y2;
}

static float dist_point_to_segment(float px, float py, float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len_sq = dx*dx + dy*dy;
    if (len_sq < 1e-6f) {
        float ddx = px - x1; float ddy = py - y1;
        return sqrtf(ddx*ddx + ddy*ddy);
    }
    float t = ((px - x1)*dx + (py - y1)*dy) / len_sq;
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
    float proj_x = x1 + t*dx; float proj_y = y1 + t*dy;
    float ddx = px - proj_x; float ddy = py - proj_y;
    return sqrtf(ddx*ddx + ddy*ddy);
}

static int line_hit_test(const Shape* s, float x, float y, float tolerance)
{
    LineImpl* line = (LineImpl*)s->impl;
    return (dist_point_to_segment(x, y, line->p1[0], line->p1[1],
                                line->p2[0], line->p2[1]) <= tolerance) ? 1 : 0;
}

static void line_get_geometry(const Shape* s, float** out_vertices, int* out_count)
{
    LineImpl* line = (LineImpl*)s->impl;
    float* verts = (float*)malloc(4 * sizeof(float));
    verts[0] = line->p1[0]; verts[1] = line->p1[1];
    verts[2] = line->p2[0]; verts[3] = line->p2[1];
    *out_vertices = verts;
    *out_count = 2;  /* 2 vertices for LINE */
}

static ShapeVTable line_vtable = {
    .name = "LINE",
    .destroy = line_destroy,
    .compute_bounds = line_bounds,
    .hit_test = line_hit_test,
    .get_geometry = line_get_geometry,
};

/* ---------- CIRCLE ---------- */
static void circle_destroy(Shape* s)
{
    if (!s) return;
    free(s->impl);
    free(s);
}

static void circle_bounds(const Shape* s, float* minX, float* minY, float* maxX, float* maxY)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    *minX = c->center[0] - c->radius;
    *minY = c->center[1] - c->radius;
    *maxX = c->center[0] + c->radius;
    *maxY = c->center[1] + c->radius;
}

static int circle_hit_test(const Shape* s, float x, float y, float tolerance)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    float dx = x - c->center[0];
    float dy = y - c->center[1];
    float dist = sqrtf(dx*dx + dy*dy);
    return (fabsf(dist - c->radius) <= tolerance) ? 1 : 0;
}

static void circle_get_geometry(const Shape* s, float** out_vertices, int* out_count)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    const int SEGMENTS = 64;
    float* verts = (float*)malloc(SEGMENTS * 2 * sizeof(float));
    for (int i = 0; i < SEGMENTS; i++) {
        float angle = (float)i / SEGMENTS * 2.0f * 3.14159265f;
        verts[i*2] = c->center[0] + c->radius * cosf(angle);
        verts[i*2+1] = c->center[1] + c->radius * sinf(angle);
    }
    *out_vertices = verts;
    *out_count = SEGMENTS;
}

static ShapeVTable circle_vtable = {
    .name = "CIRCLE",
    .destroy = circle_destroy,
    .compute_bounds = circle_bounds,
    .hit_test = circle_hit_test,
    .get_geometry = circle_get_geometry,
};

/* ---------- RECTANGLE ---------- */
static void rect_destroy(Shape* s)
{
    if (!s) return;
    free(s->impl);
    free(s);
}

static void rect_bounds(const Shape* s, float* minX, float* minY, float* maxX, float* maxY)
{
    RectImpl* r = (RectImpl*)s->impl;
    float x1 = r->min[0], y1 = r->min[1];
    float x2 = r->max[0], y2 = r->max[1];
    *minX = (x1 < x2) ? x1 : x2;
    *minY = (y1 < y2) ? y1 : y2;
    *maxX = (x1 > x2) ? x1 : x2;
    *maxY = (y1 > y2) ? y1 : y2;
}

static int rect_hit_test(const Shape* s, float x, float y, float tolerance)
{
    RectImpl* r = (RectImpl*)s->impl;
    float x1 = r->min[0], y1 = r->min[1];
    float x2 = r->max[0], y2 = r->max[1];
    float min_x = (x1 < x2) ? x1 : x2;
    float min_y = (y1 < y2) ? y1 : y2;
    float max_x = (x1 > x2) ? x1 : x2;
    float max_y = (y1 > y2) ? y1 : y2;
    return (x >= min_x - tolerance && x <= max_x + tolerance &&
            y >= min_y - tolerance && y <= max_y + tolerance) ? 1 : 0;
}

static void rect_get_geometry(const Shape* s, float** out_vertices, int* out_count)
{
    RectImpl* r = (RectImpl*)s->impl;
    float* verts = (float*)malloc(8 * sizeof(float));
    verts[0] = r->min[0]; verts[1] = r->min[1];  /* BL */
    verts[2] = r->max[0]; verts[3] = r->min[1];  /* BR */
    verts[4] = r->max[0]; verts[5] = r->max[1];  /* TR */
    verts[6] = r->min[0]; verts[7] = r->max[1];  /* TL */
    *out_vertices = verts;
    *out_count = 4;
}

static ShapeVTable rect_vtable = {
    .name = "RECT",
    .destroy = rect_destroy,
    .compute_bounds = rect_bounds,
    .hit_test = rect_hit_test,
    .get_geometry = rect_get_geometry,
};

/* ---------- Internal constructors ---------- */
static Shape* shape_create_line_internal(float x1, float y1, float x2, float y2,
                                        float r, float g, float b, float a,
                                        float line_width)
{
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (!s) return NULL;
    LineImpl* impl = (LineImpl*)malloc(sizeof(LineImpl));
    if (!impl) { free(s); return NULL; }
    impl->p1[0] = x1; impl->p1[1] = y1;
    impl->p2[0] = x2; impl->p2[1] = y2;
    s->vtable = &line_vtable;
    s->impl = impl;
    s->color[0] = r; s->color[1] = g; s->color[2] = b; s->color[3] = a;
    s->line_width = line_width;
    return s;
}

static Shape* shape_create_circle_internal(float cx, float cy, float radius,
                                         float r, float g, float b, float a,
                                         float line_width)
{
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (!s) return NULL;
    CircleImpl* impl = (CircleImpl*)malloc(sizeof(CircleImpl));
    if (!impl) { free(s); return NULL; }
    impl->center[0] = cx; impl->center[1] = cy;
    impl->radius = radius;
    s->vtable = &circle_vtable;
    s->impl = impl;
    s->color[0] = r; s->color[1] = g; s->color[2] = b; s->color[3] = a;
    s->line_width = line_width;
    return s;
}

static Shape* shape_create_rect_internal(float x1, float y1, float x2, float y2,
                                       float r, float g, float b, float a,
                                       float line_width)
{
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (!s) return NULL;
    RectImpl* impl = (RectImpl*)malloc(sizeof(RectImpl));
    if (!impl) { free(s); return NULL; }
    impl->min[0] = x1; impl->min[1] = y1;
    impl->max[0] = x2; impl->max[1] = y2;
    s->vtable = &rect_vtable;
    s->impl = impl;
    s->color[0] = r; s->color[1] = g; s->color[2] = b; s->color[3] = a;
    s->line_width = line_width;
    return s;
}

/* ---------- Public API ---------- */
Shape* shape_create(const char* type_name,
                    float r, float g, float b, float a,
                    float line_width)
{
    if (strcmp(type_name, "LINE") == 0) {
        return shape_create_line_internal(0, 0, 0, 0, r, g, b, a, line_width);
    } else if (strcmp(type_name, "CIRCLE") == 0) {
        return shape_create_circle_internal(0, 0, 0.1f, r, g, b, a, line_width);
    } else if (strcmp(type_name, "RECT") == 0) {
        return shape_create_rect_internal(0, 0, 0, 0, r, g, b, a, line_width);
    }
    fprintf(stderr, "[Shape] Unknown type: '%s'\n", type_name);
    return NULL;
}

void shape_destroy(Shape* s)
{
    if (s && s->vtable && s->vtable->destroy) {
        s->vtable->destroy(s);
    }
}

void shape_get_bounds(const Shape* s, float* minX, float* minY, float* maxX, float* maxY)
{
    if (s && s->vtable && s->vtable->compute_bounds) {
        s->vtable->compute_bounds(s, minX, minY, maxX, maxY);
    } else {
        *minX = *minY = *maxX = *maxY = 0.0f;
    }
}

int shape_hit_test(const Shape* s, float x, float y, float tolerance)
{
    if (!s || !s->vtable || !s->vtable->hit_test) return 0;
    return s->vtable->hit_test(s, x, y, tolerance);
}

void shape_get_geometry(const Shape* s, float** out_vertices, int* out_count)
{
    if (s && s->vtable && s->vtable->get_geometry) {
        s->vtable->get_geometry(s, out_vertices, out_count);
    } else {
        *out_vertices = NULL;
        *out_count = 0;
    }
}

/* ---------- Registry registration (called once at init) ---------- */
void shape_register_all(void)
{
    shape_register("LINE", &line_vtable);
    shape_register("CIRCLE", &circle_vtable);
    shape_register("RECT", &rect_vtable);
}
