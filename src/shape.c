#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <core/shape.h>
#include <core/shape_impl.h>
#include <core/shape_registry.h>
#include <core/macros.h>

/* =============================================================================
 * Concrete vtable implementations
 * =============================================================================
 */

static void shape_bump_revision(Shape* s)
{
    if (!s) return;
    s->revision++;
}

/* ---------- Default property access (shared by all shapes) ---------- */
static int default_get_property(const Shape* s, const char* key, float* out_value)
{
    if (strcmp(key, "color_r") == 0) { *out_value = s->color[0]; return 1; }
    if (strcmp(key, "color_g") == 0) { *out_value = s->color[1]; return 1; }
    if (strcmp(key, "color_b") == 0) { *out_value = s->color[2]; return 1; }
    if (strcmp(key, "color_a") == 0) { *out_value = s->color[3]; return 1; }
    if (strcmp(key, "line_width") == 0) { *out_value = s->line_width; return 1; }
    return 0;
}

static int default_set_property(Shape* s, const char* key, float value)
{
    if (strcmp(key, "color_r") == 0) { s->color[0] = value; return 1; }
    if (strcmp(key, "color_g") == 0) { s->color[1] = value; return 1; }
    if (strcmp(key, "color_b") == 0) { s->color[2] = value; return 1; }
    if (strcmp(key, "color_a") == 0) { s->color[3] = value; return 1; }
    if (strcmp(key, "line_width") == 0) { s->line_width = value; return 1; }
    return 0;
}

static void line_translate(Shape* s, float dx, float dy)
{
    LineImpl* line = (LineImpl*)s->impl;
    line->p1[0] += dx; line->p1[1] += dy;
    line->p2[0] += dx; line->p2[1] += dy;
}

static int line_get_property(const Shape* s, const char* key, float* out_value)
{
    LineImpl* line = (LineImpl*)s->impl;
    if (strcmp(key, "p1_x") == 0) { *out_value = line->p1[0]; return 1; }
    if (strcmp(key, "p1_y") == 0) { *out_value = line->p1[1]; return 1; }
    if (strcmp(key, "p2_x") == 0) { *out_value = line->p2[0]; return 1; }
    if (strcmp(key, "p2_y") == 0) { *out_value = line->p2[1]; return 1; }
    if (strcmp(key, "length") == 0) {
        float dx = line->p2[0] - line->p1[0];
        float dy = line->p2[1] - line->p1[1];
        *out_value = sqrtf(dx*dx + dy*dy);
        return 1;
    }
    return default_get_property(s, key, out_value);
}

static int line_set_property(Shape* s, const char* key, float value)
{
    LineImpl* line = (LineImpl*)s->impl;
    if (strcmp(key, "p1_x") == 0) { line->p1[0] = value; return 1; }
    if (strcmp(key, "p1_y") == 0) { line->p1[1] = value; return 1; }
    if (strcmp(key, "p2_x") == 0) { line->p2[0] = value; return 1; }
    if (strcmp(key, "p2_y") == 0) { line->p2[1] = value; return 1; }
    return default_set_property(s, key, value);
}

/* ---------- LINE ---------- */
static void line_destroy(Shape* s)
{
    if (!s) return;
    SAFE_FREE(s->impl);
    SAFE_FREE(s);
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

static int line_get_vertex_count(const Shape* s)
{
    (void)s;
    return 2;
}

static void line_write_geometry(const Shape* s, float* out_vertices)
{
    LineImpl* line = (LineImpl*)s->impl;
    out_vertices[0] = line->p1[0]; out_vertices[1] = line->p1[1];
    out_vertices[2] = line->p2[0]; out_vertices[3] = line->p2[1];
}

static ShapeVTable line_vtable = {
    .name = "LINE",
    .destroy = line_destroy,
    .compute_bounds = line_bounds,
    .hit_test = line_hit_test,
    .get_vertex_count = line_get_vertex_count,
    .write_geometry = line_write_geometry,
    .translate = line_translate,
    .get_property = line_get_property,
    .set_property = line_set_property,
};

/* ---------- CIRCLE ---------- */
static void circle_destroy(Shape* s)
{
    if (!s) return;
    SAFE_FREE(s->impl);
    SAFE_FREE(s);
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

static int circle_get_vertex_count(const Shape* s)
{
    (void)s;
    const int SEGMENTS = 64;
    return SEGMENTS;
}

static void circle_write_geometry(const Shape* s, float* out_vertices)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    const int SEGMENTS = 64;
    for (int i = 0; i < SEGMENTS; i++) {
        float angle = (float)i / SEGMENTS * 2.0f * 3.14159265f;
        out_vertices[i*2] = c->center[0] + c->radius * cosf(angle);
        out_vertices[i*2+1] = c->center[1] + c->radius * sinf(angle);
    }
}

static void circle_translate(Shape* s, float dx, float dy)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    c->center[0] += dx;
    c->center[1] += dy;
}

static int circle_get_property(const Shape* s, const char* key, float* out_value)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    if (strcmp(key, "center_x") == 0) { *out_value = c->center[0]; return 1; }
    if (strcmp(key, "center_y") == 0) { *out_value = c->center[1]; return 1; }
    if (strcmp(key, "radius") == 0) { *out_value = c->radius; return 1; }
    return default_get_property(s, key, out_value);
}

static int circle_set_property(Shape* s, const char* key, float value)
{
    CircleImpl* c = (CircleImpl*)s->impl;
    if (strcmp(key, "center_x") == 0) { c->center[0] = value; return 1; }
    if (strcmp(key, "center_y") == 0) { c->center[1] = value; return 1; }
    if (strcmp(key, "radius") == 0) { c->radius = value; return 1; }
    return default_set_property(s, key, value);
}

static ShapeVTable circle_vtable = {
    .name = "CIRCLE",
    .destroy = circle_destroy,
    .compute_bounds = circle_bounds,
    .hit_test = circle_hit_test,
    .get_vertex_count = circle_get_vertex_count,
    .write_geometry = circle_write_geometry,
    .translate = circle_translate,
    .get_property = circle_get_property,
    .set_property = circle_set_property,
};

/* ---------- RECTANGLE ---------- */
static void rect_destroy(Shape* s)
{
    if (!s) return;
    SAFE_FREE(s->impl);
    SAFE_FREE(s);
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

static int rect_get_vertex_count(const Shape* s)
{
    (void)s;
    return 4;
}

static void rect_write_geometry(const Shape* s, float* out_vertices)
{
    RectImpl* r = (RectImpl*)s->impl;
    out_vertices[0] = r->min[0]; out_vertices[1] = r->min[1];  /* BL */
    out_vertices[2] = r->max[0]; out_vertices[3] = r->min[1];  /* BR */
    out_vertices[4] = r->max[0]; out_vertices[5] = r->max[1];  /* TR */
    out_vertices[6] = r->min[0]; out_vertices[7] = r->max[1];  /* TL */
}

static void rect_translate(Shape* s, float dx, float dy)
{
    RectImpl* r = (RectImpl*)s->impl;
    r->min[0] += dx; r->min[1] += dy;
    r->max[0] += dx; r->max[1] += dy;
}

static int rect_get_property(const Shape* s, const char* key, float* out_value)
{
    RectImpl* r = (RectImpl*)s->impl;
    if (strcmp(key, "x") == 0) { *out_value = r->min[0]; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = r->min[1]; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = r->max[0] - r->min[0]; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = r->max[1] - r->min[1]; return 1; }
    return default_get_property(s, key, out_value);
}

static int rect_set_property(Shape* s, const char* key, float value)
{
    RectImpl* r = (RectImpl*)s->impl;
    if (strcmp(key, "x") == 0) { r->min[0] = value; return 1; }
    if (strcmp(key, "y") == 0) { r->min[1] = value; return 1; }
    if (strcmp(key, "width") == 0) { r->max[0] = r->min[0] + value; return 1; }
    if (strcmp(key, "height") == 0) { r->max[1] = r->min[1] + value; return 1; }
    return default_set_property(s, key, value);
}

static ShapeVTable rect_vtable = {
    .name = "RECT",
    .destroy = rect_destroy,
    .compute_bounds = rect_bounds,
    .hit_test = rect_hit_test,
    .get_vertex_count = rect_get_vertex_count,
    .write_geometry = rect_write_geometry,
    .translate = rect_translate,
    .get_property = rect_get_property,
    .set_property = rect_set_property,
};

/* ---------- Internal constructors ---------- */
static Shape* shape_create_line_internal(float x1, float y1, float x2, float y2,
                                        float r, float g, float b, float a,
                                        float line_width)
{
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (UNLIKELY(!s)) return NULL;
    LineImpl* impl = (LineImpl*)malloc(sizeof(LineImpl));
    if (UNLIKELY(!impl)) { SAFE_FREE(s); return NULL; }
    impl->p1[0] = x1; impl->p1[1] = y1;
    impl->p2[0] = x2; impl->p2[1] = y2;
    s->vtable = &line_vtable;
    s->impl = impl;
    s->color[0] = r; s->color[1] = g; s->color[2] = b; s->color[3] = a;
    s->line_width = line_width;
    s->revision = 1;
    return s;
}

static Shape* shape_create_circle_internal(float cx, float cy, float radius,
                                         float r, float g, float b, float a,
                                         float line_width)
{
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (UNLIKELY(!s)) return NULL;
    CircleImpl* impl = (CircleImpl*)malloc(sizeof(CircleImpl));
    if (UNLIKELY(!impl)) { SAFE_FREE(s); return NULL; }
    impl->center[0] = cx; impl->center[1] = cy;
    impl->radius = radius;
    s->vtable = &circle_vtable;
    s->impl = impl;
    s->color[0] = r; s->color[1] = g; s->color[2] = b; s->color[3] = a;
    s->line_width = line_width;
    s->revision = 1;
    return s;
}

static Shape* shape_create_rect_internal(float x1, float y1, float x2, float y2,
                                       float r, float g, float b, float a,
                                       float line_width)
{
    Shape* s = (Shape*)malloc(sizeof(Shape));
    if (UNLIKELY(!s)) return NULL;
    RectImpl* impl = (RectImpl*)malloc(sizeof(RectImpl));
    if (UNLIKELY(!impl)) { SAFE_FREE(s); return NULL; }
    impl->min[0] = x1; impl->min[1] = y1;
    impl->max[0] = x2; impl->max[1] = y2;
    s->vtable = &rect_vtable;
    s->impl = impl;
    s->color[0] = r; s->color[1] = g; s->color[2] = b; s->color[3] = a;
    s->line_width = line_width;
    s->revision = 1;
    return s;
}

static Shape* line_create_default(float r, float g, float b, float a, float line_width)
{
    return shape_create_line_internal(0.0f, 0.0f, 0.0f, 0.0f, r, g, b, a, line_width);
}

static Shape* circle_create_default(float r, float g, float b, float a, float line_width)
{
    return shape_create_circle_internal(0.0f, 0.0f, 0.1f, r, g, b, a, line_width);
}

static Shape* rect_create_default(float r, float g, float b, float a, float line_width)
{
    return shape_create_rect_internal(0.0f, 0.0f, 0.0f, 0.0f, r, g, b, a, line_width);
}

/* ---------- Public API ---------- */
Shape* shape_create(const char* type_name,
                    float r, float g, float b, float a,
                    float line_width)
{
    Shape* shape = shape_registry_create(type_name, r, g, b, a, line_width);
    if (UNLIKELY(!shape)) {
        LOG_ERROR_F("Unknown shape type: '%s'", type_name ? type_name : "(null)");
    }
    return shape;
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

int shape_get_vertex_count(const Shape* s)
{
    if (!s || !s->vtable || !s->vtable->get_vertex_count) return 0;
    return s->vtable->get_vertex_count(s);
}

void shape_write_geometry(const Shape* s, float* out_vertices)
{
    if (!s || !s->vtable || !s->vtable->write_geometry || !out_vertices) return;
    s->vtable->write_geometry(s, out_vertices);
}

void shape_translate(Shape* s, float dx, float dy)
{
    if (!s || !s->vtable || !s->vtable->translate) return;
    s->vtable->translate(s, dx, dy);
    shape_bump_revision(s);
}

unsigned int shape_get_revision(const Shape* s)
{
    if (!s) return 0;
    return s->revision;
}

int shape_get_property(const Shape* s, const char* key, float* out_value)
{
    if (!s || !s->vtable || !s->vtable->get_property) return 0;
    return s->vtable->get_property(s, key, out_value);
}

int shape_set_property(Shape* s, const char* key, float value)
{
    if (!s || !s->vtable || !s->vtable->set_property) return 0;
    if (s->vtable->set_property(s, key, value)) {
        shape_bump_revision(s);
        return 1;
    }
    return 0;
}

/* ---------- Registry registration (called once at init) ---------- */
void shape_register_all(void)
{
    shape_register("LINE", line_create_default, &line_vtable);
    shape_register("CIRCLE", circle_create_default, &circle_vtable);
    shape_register("RECT", rect_create_default, &rect_vtable);
}
