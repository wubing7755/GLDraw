/**
 * @file object.c
 * @brief Polymorphic drawable object implementations (line/rect/ellipse).
 *
 * Role in project:
 * - Implements per-shape vtable functions: bounds, hit test, path extraction,
 *   scalar property access, and translation.
 * - Provides factory and wrapper APIs used throughout the editor.
 *
 * Module relationships:
 * - Consumed by document/history/render/canvas/persistence modules.
 * - Uses math helpers for geometry operations.
 */
#include <document/object.h>

#include <base/math2d.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ELLIPSE_SEGMENTS 64

typedef struct {
    Vec2 p1;
    Vec2 p2;
} LineData;

typedef struct {
    RectF rect;
} RectData;

typedef struct {
    RectF bounds;
} EllipseData;

/** Bump per-object revision after successful mutations. Complexity: `O(1)`. */
static void object_bump_revision(GraphicObject* object)
{
    if (object) {
        object->revision++;
    }
}

/** Read style scalar by key. Complexity: `O(1)` with small fixed key set. */
static int style_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    if (strcmp(key, "stroke_r") == 0) { *out_value = object->style.stroke_color.r; return 1; }
    if (strcmp(key, "stroke_g") == 0) { *out_value = object->style.stroke_color.g; return 1; }
    if (strcmp(key, "stroke_b") == 0) { *out_value = object->style.stroke_color.b; return 1; }
    if (strcmp(key, "stroke_a") == 0) { *out_value = object->style.stroke_color.a; return 1; }
    if (strcmp(key, "stroke_width") == 0) { *out_value = object->style.stroke_width; return 1; }
    return 0;
}

/** Write style scalar by key. Complexity: `O(1)` with small fixed key set. */
static int style_set_scalar(GraphicObject* object, const char* key, float value)
{
    if (strcmp(key, "stroke_r") == 0) { object->style.stroke_color.r = value; return 1; }
    if (strcmp(key, "stroke_g") == 0) { object->style.stroke_color.g = value; return 1; }
    if (strcmp(key, "stroke_b") == 0) { object->style.stroke_color.b = value; return 1; }
    if (strcmp(key, "stroke_a") == 0) { object->style.stroke_color.a = value; return 1; }
    if (strcmp(key, "stroke_width") == 0) { object->style.stroke_width = value; return 1; }
    return 0;
}

/** Line object destructor; releases `impl` then wrapper object. */
static void line_destroy(GraphicObject* object)
{
    free(object->impl);
    free(object);
}

/** Translate line endpoints by world delta. Complexity: `O(1)`. */
static void line_translate(GraphicObject* object, Vec2 delta)
{
    LineData* line = (LineData*)object->impl;
    line->p1 = vec2_add(line->p1, delta);
    line->p2 = vec2_add(line->p2, delta);
}

/** Axis-aligned bounds for line segment. Complexity: `O(1)`. */
static RectF line_bounds(const GraphicObject* object)
{
    const LineData* line = (const LineData*)object->impl;
    float min_x = (line->p1.x < line->p2.x) ? line->p1.x : line->p2.x;
    float min_y = (line->p1.y < line->p2.y) ? line->p1.y : line->p2.y;
    float max_x = (line->p1.x > line->p2.x) ? line->p1.x : line->p2.x;
    float max_y = (line->p1.y > line->p2.y) ? line->p1.y : line->p2.y;
    RectF bounds = {min_x, min_y, max_x - min_x, max_y - min_y};
    return bounds;
}

/**
 * @brief Distance from point to segment AB.
 * Why projection:
 * - Projecting onto AB and clamping `t` gives nearest on-segment point robustly.
 * Complexity: `O(1)`.
 */
static float line_distance_to_segment(Vec2 point, Vec2 a, Vec2 b)
{
    Vec2 ab = vec2_sub(b, a);
    float ab_len_sq = vec2_length_sq(ab);
    float t = 0.0f;
    Vec2 nearest = {0.0f, 0.0f};

    if (ab_len_sq <= 1e-6f) {
        return vec2_length(vec2_sub(point, a));
    }

    t = vec2_dot(vec2_sub(point, a), ab) / ab_len_sq;
    nearest = vec2_add(a, vec2_scale(ab, clampf(t, 0.0f, 1.0f)));
    return vec2_length(vec2_sub(point, nearest));
}

/** Line hit-test uses geometric point-to-segment distance. Complexity: `O(1)`. */
static int line_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    const LineData* line = (const LineData*)object->impl;
    return line_distance_to_segment(point, line->p1, line->p2) <= tolerance;
}

/** Line path emits two points. */
static int line_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return 2;
}

/** Write line path points into caller buffer. */
static void line_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const LineData* line = (const LineData*)object->impl;
    out_points[0] = line->p1;
    out_points[1] = line->p2;
}

/** Read line scalar property or fallback to style scalar. */
static int line_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const LineData* line = (const LineData*)object->impl;
    if (strcmp(key, "x1") == 0) { *out_value = line->p1.x; return 1; }
    if (strcmp(key, "y1") == 0) { *out_value = line->p1.y; return 1; }
    if (strcmp(key, "x2") == 0) { *out_value = line->p2.x; return 1; }
    if (strcmp(key, "y2") == 0) { *out_value = line->p2.y; return 1; }
    return style_get_scalar(object, key, out_value);
}

/** Write line scalar property or fallback to style scalar. */
static int line_set_scalar(GraphicObject* object, const char* key, float value)
{
    LineData* line = (LineData*)object->impl;
    if (strcmp(key, "x1") == 0) { line->p1.x = value; return 1; }
    if (strcmp(key, "y1") == 0) { line->p1.y = value; return 1; }
    if (strcmp(key, "x2") == 0) { line->p2.x = value; return 1; }
    if (strcmp(key, "y2") == 0) { line->p2.y = value; return 1; }
    return style_set_scalar(object, key, value);
}

/** Rectangle object destructor. */
static void rect_destroy(GraphicObject* object)
{
    free(object->impl);
    free(object);
}

/** Translate rectangle origin by world delta. */
static void rect_translate(GraphicObject* object, Vec2 delta)
{
    RectData* rect = (RectData*)object->impl;
    rect->rect.x += delta.x;
    rect->rect.y += delta.y;
}

/** Return rectangle bounds directly from payload. */
static RectF rect_bounds(const GraphicObject* object)
{
    const RectData* rect = (const RectData*)object->impl;
    return rect->rect;
}

/** Rectangle hit-test expands bounds by tolerance for easier selection. */
static int rect_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    RectF bounds = rect_bounds(object);
    bounds.x -= tolerance;
    bounds.y -= tolerance;
    bounds.w += tolerance * 2.0f;
    bounds.h += tolerance * 2.0f;
    return rectf_contains_point(&bounds, point);
}

/** Rectangle polyline is closed with 5 points. */
static int rect_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return 5;
}

/** Emit rectangle corners plus closing point. */
static void rect_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const RectData* rect = (const RectData*)object->impl;
    out_points[0] = vec2_make(rect->rect.x, rect->rect.y);
    out_points[1] = vec2_make(rect->rect.x + rect->rect.w, rect->rect.y);
    out_points[2] = vec2_make(rect->rect.x + rect->rect.w, rect->rect.y + rect->rect.h);
    out_points[3] = vec2_make(rect->rect.x, rect->rect.y + rect->rect.h);
    out_points[4] = out_points[0];
}

/** Read rectangle scalar property or style scalar. */
static int rect_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const RectData* rect = (const RectData*)object->impl;
    if (strcmp(key, "x") == 0) { *out_value = rect->rect.x; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = rect->rect.y; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = rect->rect.w; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = rect->rect.h; return 1; }
    return style_get_scalar(object, key, out_value);
}

/** Write rectangle scalar property or style scalar. */
static int rect_set_scalar(GraphicObject* object, const char* key, float value)
{
    RectData* rect = (RectData*)object->impl;
    if (strcmp(key, "x") == 0) { rect->rect.x = value; return 1; }
    if (strcmp(key, "y") == 0) { rect->rect.y = value; return 1; }
    if (strcmp(key, "width") == 0) { rect->rect.w = value; return 1; }
    if (strcmp(key, "height") == 0) { rect->rect.h = value; return 1; }
    return style_set_scalar(object, key, value);
}

/** Ellipse object destructor. */
static void ellipse_destroy(GraphicObject* object)
{
    free(object->impl);
    free(object);
}

/** Translate ellipse bounds origin by world delta. */
static void ellipse_translate(GraphicObject* object, Vec2 delta)
{
    EllipseData* ellipse = (EllipseData*)object->impl;
    ellipse->bounds.x += delta.x;
    ellipse->bounds.y += delta.y;
}

/** Return ellipse axis-aligned bounds. */
static RectF ellipse_bounds(const GraphicObject* object)
{
    const EllipseData* ellipse = (const EllipseData*)object->impl;
    return ellipse->bounds;
}

/** Ellipse hit-test in normalized ellipse space. Complexity: `O(1)`. */
static int ellipse_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    RectF bounds = ellipse_bounds(object);
    float rx = (bounds.w * 0.5f) + tolerance;
    float ry = (bounds.h * 0.5f) + tolerance;
    Vec2 center = vec2_make(bounds.x + bounds.w * 0.5f, bounds.y + bounds.h * 0.5f);
    float nx = 0.0f;
    float ny = 0.0f;

    if (rx <= 1e-6f || ry <= 1e-6f) {
        return 0;
    }

    nx = (point.x - center.x) / rx;
    ny = (point.y - center.y) / ry;
    return (nx * nx + ny * ny) <= 1.0f;
}

/** Ellipse polyline uses fixed segment count plus closure point. */
static int ellipse_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return ELLIPSE_SEGMENTS + 1;
}

/**
 * @brief Emit sampled ellipse perimeter.
 * Risk note:
 * - Caller must provide at least `ELLIPSE_SEGMENTS + 1` output points.
 */
static void ellipse_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const EllipseData* ellipse = (const EllipseData*)object->impl;
    float rx = ellipse->bounds.w * 0.5f;
    float ry = ellipse->bounds.h * 0.5f;
    Vec2 center = vec2_make(ellipse->bounds.x + rx, ellipse->bounds.y + ry);
    int i = 0;

    for (i = 0; i < ELLIPSE_SEGMENTS; ++i) {
        float t = (float)i / (float)ELLIPSE_SEGMENTS;
        float angle = t * 6.28318530718f;
        out_points[i] = vec2_make(center.x + cosf(angle) * rx, center.y + sinf(angle) * ry);
    }

    out_points[ELLIPSE_SEGMENTS] = out_points[0];
}

/** Read ellipse scalar property or style scalar. */
static int ellipse_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const EllipseData* ellipse = (const EllipseData*)object->impl;
    if (strcmp(key, "x") == 0) { *out_value = ellipse->bounds.x; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = ellipse->bounds.y; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = ellipse->bounds.w; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = ellipse->bounds.h; return 1; }
    return style_get_scalar(object, key, out_value);
}

/** Write ellipse scalar property or style scalar. */
static int ellipse_set_scalar(GraphicObject* object, const char* key, float value)
{
    EllipseData* ellipse = (EllipseData*)object->impl;
    if (strcmp(key, "x") == 0) { ellipse->bounds.x = value; return 1; }
    if (strcmp(key, "y") == 0) { ellipse->bounds.y = value; return 1; }
    if (strcmp(key, "width") == 0) { ellipse->bounds.w = value; return 1; }
    if (strcmp(key, "height") == 0) { ellipse->bounds.h = value; return 1; }
    return style_set_scalar(object, key, value);
}

static const GraphicObjectVTable g_line_vtable = {
    "Line",
    line_destroy,
    line_translate,
    line_bounds,
    line_hit_test,
    line_get_path_point_count,
    line_write_path_points,
    line_get_scalar,
    line_set_scalar
};

static const GraphicObjectVTable g_rect_vtable = {
    "Rectangle",
    rect_destroy,
    rect_translate,
    rect_bounds,
    rect_hit_test,
    rect_get_path_point_count,
    rect_write_path_points,
    rect_get_scalar,
    rect_set_scalar
};

static const GraphicObjectVTable g_ellipse_vtable = {
    "Ellipse",
    ellipse_destroy,
    ellipse_translate,
    ellipse_bounds,
    ellipse_hit_test,
    ellipse_get_path_point_count,
    ellipse_write_path_points,
    ellipse_get_scalar,
    ellipse_set_scalar
};

/** Allocate common object wrapper; takes ownership of `impl` on success. */
static GraphicObject* object_alloc(GraphicObjectType type,
                                   const GraphicObjectVTable* vtable,
                                   void* impl,
                                   GraphicStyle style)
{
    GraphicObject* object = (GraphicObject*)calloc(1, sizeof(*object));
    if (!object) {
        free(impl);
        return NULL;
    }

    object->type = type;
    object->vtable = vtable;
    object->impl = impl;
    object->style = style;
    object->revision = 1;
    return object;
}

/** Return default stroke style for new objects. */
GraphicStyle object_default_style(void)
{
    GraphicStyle style;
    style.stroke_color.r = 0.15f;
    style.stroke_color.g = 0.78f;
    style.stroke_color.b = 0.95f;
    style.stroke_color.a = 1.0f;
    style.stroke_width = 2.0f;
    return style;
}

/** Return debug/display name by object type. */
const char* object_type_name(GraphicObjectType type)
{
    switch (type) {
    case GRAPHIC_OBJECT_LINE:
        return "Line";
    case GRAPHIC_OBJECT_RECT:
        return "Rectangle";
    case GRAPHIC_OBJECT_ELLIPSE:
        return "Ellipse";
    default:
        return "Unknown";
    }
}

/** Create line object from two endpoints. */
GraphicObject* object_create_line(Vec2 p1, Vec2 p2, GraphicStyle style)
{
    LineData* line = (LineData*)calloc(1, sizeof(*line));
    if (!line) {
        return NULL;
    }
    line->p1 = p1;
    line->p2 = p2;
    return object_alloc(GRAPHIC_OBJECT_LINE, &g_line_vtable, line, style);
}

/** Create rectangle object from bounds. */
GraphicObject* object_create_rect(RectF rect, GraphicStyle style)
{
    RectData* data = (RectData*)calloc(1, sizeof(*data));
    if (!data) {
        return NULL;
    }
    data->rect = rect;
    return object_alloc(GRAPHIC_OBJECT_RECT, &g_rect_vtable, data, style);
}

/** Create ellipse object from bounds. */
GraphicObject* object_create_ellipse(RectF bounds, GraphicStyle style)
{
    EllipseData* data = (EllipseData*)calloc(1, sizeof(*data));
    if (!data) {
        return NULL;
    }
    data->bounds = bounds;
    return object_alloc(GRAPHIC_OBJECT_ELLIPSE, &g_ellipse_vtable, data, style);
}

/**
 * @brief Deep clone object by concrete type.
 * @return New object with copied ID/style/revision, or `NULL` on failure.
 */
GraphicObject* object_clone(const GraphicObject* object)
{
    GraphicObject* clone = NULL;

    if (!object) {
        return NULL;
    }

    switch (object->type) {
    case GRAPHIC_OBJECT_LINE:
    {
        const LineData* line = (const LineData*)object->impl;
        clone = object_create_line(line->p1, line->p2, object->style);
        break;
    }
    case GRAPHIC_OBJECT_RECT:
    {
        const RectData* rect = (const RectData*)object->impl;
        clone = object_create_rect(rect->rect, object->style);
        break;
    }
    case GRAPHIC_OBJECT_ELLIPSE:
    {
        const EllipseData* ellipse = (const EllipseData*)object->impl;
        clone = object_create_ellipse(ellipse->bounds, object->style);
        break;
    }
    default:
        return NULL;
    }

    if (!clone) {
        return NULL;
    }

    clone->id = object->id;
    clone->revision = object->revision;
    return clone;
}

/** Dispatch destruction through vtable. */
void object_destroy(GraphicObject* object)
{
    if (object && object->vtable && object->vtable->destroy) {
        object->vtable->destroy(object);
    }
}

/** Translate object and bump revision on success. */
void object_translate(GraphicObject* object, Vec2 delta)
{
    if (!object || !object->vtable || !object->vtable->translate) {
        return;
    }
    object->vtable->translate(object, delta);
    object_bump_revision(object);
}

/** Query object bounds via vtable, fallback to empty rect. */
RectF object_get_bounds(const GraphicObject* object)
{
    RectF empty = {0.0f, 0.0f, 0.0f, 0.0f};
    if (!object || !object->vtable || !object->vtable->get_bounds) {
        return empty;
    }
    return object->vtable->get_bounds(object);
}

/** Hit-test object via vtable. */
int object_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    if (!object || !object->vtable || !object->vtable->hit_test) {
        return 0;
    }
    return object->vtable->hit_test(object, point, tolerance);
}

/** Query path sample count via vtable. */
int object_get_path_point_count(const GraphicObject* object)
{
    if (!object || !object->vtable || !object->vtable->get_path_point_count) {
        return 0;
    }
    return object->vtable->get_path_point_count(object);
}

/** Write path points via vtable. */
void object_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    if (!object || !object->vtable || !object->vtable->write_path_points) {
        return;
    }
    object->vtable->write_path_points(object, out_points);
}

/** Query scalar property via vtable. */
int object_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    if (!object || !object->vtable || !object->vtable->get_scalar) {
        return 0;
    }
    return object->vtable->get_scalar(object, key, out_value);
}

/** Set scalar property via vtable and bump revision on success. */
int object_set_scalar(GraphicObject* object, const char* key, float value)
{
    if (!object || !object->vtable || !object->vtable->set_scalar) {
        return 0;
    }
    if (!object->vtable->set_scalar(object, key, value)) {
        return 0;
    }
    object_bump_revision(object);
    return 1;
}

/** Update stroke color and revision. */
void object_set_stroke_color(GraphicObject* object, Color color)
{
    if (!object) {
        return;
    }
    object->style.stroke_color = color;
    object_bump_revision(object);
}

/** Update stroke width and revision. */
void object_set_stroke_width(GraphicObject* object, float stroke_width)
{
    if (!object) {
        return;
    }
    object->style.stroke_width = stroke_width;
    object_bump_revision(object);
}
