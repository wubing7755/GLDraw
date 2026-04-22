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

/**
 * @brief Increments the object revision.
 * @param object Object instance.
 * @return None.
 */
static void object_bump_revision(GraphicObject* object)
{
    if (object) {
        object->revision++;
    }
}

/**
 * @brief Gets a style scalar value.
 * @param object Object instance.
 * @param key Style key name.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
static int style_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    if (strcmp(key, "stroke_r") == 0) { *out_value = object->style.stroke_color.r; return 1; }
    if (strcmp(key, "stroke_g") == 0) { *out_value = object->style.stroke_color.g; return 1; }
    if (strcmp(key, "stroke_b") == 0) { *out_value = object->style.stroke_color.b; return 1; }
    if (strcmp(key, "stroke_a") == 0) { *out_value = object->style.stroke_color.a; return 1; }
    if (strcmp(key, "stroke_width") == 0) { *out_value = object->style.stroke_width; return 1; }
    return 0;
}

/**
 * @brief Sets a style scalar value.
 * @param object Object instance.
 * @param key Style key name.
 * @param value New value.
 * @return 1 on success, 0 on failure.
 */
static int style_set_scalar(GraphicObject* object, const char* key, float value)
{
    if (strcmp(key, "stroke_r") == 0) { object->style.stroke_color.r = value; return 1; }
    if (strcmp(key, "stroke_g") == 0) { object->style.stroke_color.g = value; return 1; }
    if (strcmp(key, "stroke_b") == 0) { object->style.stroke_color.b = value; return 1; }
    if (strcmp(key, "stroke_a") == 0) { object->style.stroke_color.a = value; return 1; }
    if (strcmp(key, "stroke_width") == 0) { object->style.stroke_width = value; return 1; }
    return 0;
}

/**
 * @brief Destroys a line object.
 * @param object Line object instance.
 * @return None.
 */
static void line_destroy(GraphicObject* object)
{
    free(object->impl);
    free(object);
}

/**
 * @brief Translates a line object.
 * @param object Line object instance.
 * @param delta Translation vector.
 * @return None.
 */
static void line_translate(GraphicObject* object, Vec2 delta)
{
    LineData* line = (LineData*)object->impl;
    line->p1 = vec2_add(line->p1, delta);
    line->p2 = vec2_add(line->p2, delta);
}

/**
 * @brief Gets the bounding box of a line.
 * @param object Line object instance.
 * @return Bounding rectangle.
 */
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
 * @brief Calculates distance from point to line segment.
 * @param point Point to test.
 * @param a Line segment start point.
 * @param b Line segment end point.
 * @return Distance to segment.
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

/**
 * @brief Hit tests a line object.
 * @param object Line object instance.
 * @param point Test point.
 * @param tolerance Hit tolerance.
 * @return 1 if hit, 0 otherwise.
 */
static int line_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    const LineData* line = (const LineData*)object->impl;
    return line_distance_to_segment(point, line->p1, line->p2) <= tolerance;
}

/**
 * @brief Gets the path point count for a line.
 * @param object Line object instance.
 * @return Number of path points.
 */
static int line_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return 2;
}

/**
 * @brief Writes path points for a line.
 * @param object Line object instance.
 * @param out_points Output array for path points.
 * @return None.
 */
static void line_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const LineData* line = (const LineData*)object->impl;
    out_points[0] = line->p1;
    out_points[1] = line->p2;
}

/**
 * @brief Gets a scalar property of a line.
 * @param object Line object instance.
 * @param key Property key.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
static int line_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const LineData* line = (const LineData*)object->impl;
    if (strcmp(key, "x1") == 0) { *out_value = line->p1.x; return 1; }
    if (strcmp(key, "y1") == 0) { *out_value = line->p1.y; return 1; }
    if (strcmp(key, "x2") == 0) { *out_value = line->p2.x; return 1; }
    if (strcmp(key, "y2") == 0) { *out_value = line->p2.y; return 1; }
    return style_get_scalar(object, key, out_value);
}

/**
 * @brief Sets a scalar property of a line.
 * @param object Line object instance.
 * @param key Property key.
 * @param value New value.
 * @return 1 on success, 0 on failure.
 */
static int line_set_scalar(GraphicObject* object, const char* key, float value)
{
    LineData* line = (LineData*)object->impl;
    if (strcmp(key, "x1") == 0) { line->p1.x = value; return 1; }
    if (strcmp(key, "y1") == 0) { line->p1.y = value; return 1; }
    if (strcmp(key, "x2") == 0) { line->p2.x = value; return 1; }
    if (strcmp(key, "y2") == 0) { line->p2.y = value; return 1; }
    return style_set_scalar(object, key, value);
}

/**
 * @brief Destroys a rectangle object.
 * @param object Rectangle object instance.
 * @return None.
 */
static void rect_destroy(GraphicObject* object)
{
    free(object->impl);
    free(object);
}

/**
 * @brief Translates a rectangle object.
 * @param object Rectangle object instance.
 * @param delta Translation vector.
 * @return None.
 */
static void rect_translate(GraphicObject* object, Vec2 delta)
{
    RectData* rect = (RectData*)object->impl;
    rect->rect.x += delta.x;
    rect->rect.y += delta.y;
}

/**
 * @brief Gets the bounding box of a rectangle.
 * @param object Rectangle object instance.
 * @return Bounding rectangle.
 */
static RectF rect_bounds(const GraphicObject* object)
{
    const RectData* rect = (const RectData*)object->impl;
    return rect->rect;
}

/**
 * @brief Hit tests a rectangle object.
 * @param object Rectangle object instance.
 * @param point Test point.
 * @param tolerance Hit tolerance.
 * @return 1 if hit, 0 otherwise.
 */
static int rect_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    RectF bounds = rect_bounds(object);
    bounds.x -= tolerance;
    bounds.y -= tolerance;
    bounds.w += tolerance * 2.0f;
    bounds.h += tolerance * 2.0f;
    return rectf_contains_point(&bounds, point);
}

/**
 * @brief Gets the path point count for a rectangle.
 * @param object Rectangle object instance.
 * @return Number of path points.
 */
static int rect_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return 5;
}

/**
 * @brief Writes path points for a rectangle.
 * @param object Rectangle object instance.
 * @param out_points Output array for path points.
 * @return None.
 */
static void rect_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const RectData* rect = (const RectData*)object->impl;
    out_points[0] = vec2_make(rect->rect.x, rect->rect.y);
    out_points[1] = vec2_make(rect->rect.x + rect->rect.w, rect->rect.y);
    out_points[2] = vec2_make(rect->rect.x + rect->rect.w, rect->rect.y + rect->rect.h);
    out_points[3] = vec2_make(rect->rect.x, rect->rect.y + rect->rect.h);
    out_points[4] = out_points[0];
}

/**
 * @brief Gets a scalar property of a rectangle.
 * @param object Rectangle object instance.
 * @param key Property key.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
static int rect_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const RectData* rect = (const RectData*)object->impl;
    if (strcmp(key, "x") == 0) { *out_value = rect->rect.x; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = rect->rect.y; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = rect->rect.w; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = rect->rect.h; return 1; }
    return style_get_scalar(object, key, out_value);
}

/**
 * @brief Sets a scalar property of a rectangle.
 * @param object Rectangle object instance.
 * @param key Property key.
 * @param value New value.
 * @return 1 on success, 0 on failure.
 */
static int rect_set_scalar(GraphicObject* object, const char* key, float value)
{
    RectData* rect = (RectData*)object->impl;
    if (strcmp(key, "x") == 0) { rect->rect.x = value; return 1; }
    if (strcmp(key, "y") == 0) { rect->rect.y = value; return 1; }
    if (strcmp(key, "width") == 0) { rect->rect.w = value; return 1; }
    if (strcmp(key, "height") == 0) { rect->rect.h = value; return 1; }
    return style_set_scalar(object, key, value);
}

/**
 * @brief Destroys an ellipse object.
 * @param object Ellipse object instance.
 * @return None.
 */
static void ellipse_destroy(GraphicObject* object)
{
    free(object->impl);
    free(object);
}

/**
 * @brief Translates an ellipse object.
 * @param object Ellipse object instance.
 * @param delta Translation vector.
 * @return None.
 */
static void ellipse_translate(GraphicObject* object, Vec2 delta)
{
    EllipseData* ellipse = (EllipseData*)object->impl;
    ellipse->bounds.x += delta.x;
    ellipse->bounds.y += delta.y;
}

/**
 * @brief Gets the bounding box of an ellipse.
 * @param object Ellipse object instance.
 * @return Bounding rectangle.
 */
static RectF ellipse_bounds(const GraphicObject* object)
{
    const EllipseData* ellipse = (const EllipseData*)object->impl;
    return ellipse->bounds;
}

/**
 * @brief Hit tests an ellipse object.
 * @param object Ellipse object instance.
 * @param point Test point.
 * @param tolerance Hit tolerance.
 * @return 1 if hit, 0 otherwise.
 */
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

/**
 * @brief Gets the path point count for an ellipse.
 * @param object Ellipse object instance.
 * @return Number of path points.
 */
static int ellipse_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return ELLIPSE_SEGMENTS + 1;
}

/**
 * @brief Writes path points for an ellipse.
 * @param object Ellipse object instance.
 * @param out_points Output array for path points.
 * @return None.
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

/**
 * @brief Gets a scalar property of an ellipse.
 * @param object Ellipse object instance.
 * @param key Property key.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
static int ellipse_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const EllipseData* ellipse = (const EllipseData*)object->impl;
    if (strcmp(key, "x") == 0) { *out_value = ellipse->bounds.x; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = ellipse->bounds.y; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = ellipse->bounds.w; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = ellipse->bounds.h; return 1; }
    return style_get_scalar(object, key, out_value);
}

/**
 * @brief Sets a scalar property of an ellipse.
 * @param object Ellipse object instance.
 * @param key Property key.
 * @param value New value.
 * @return 1 on success, 0 on failure.
 */
static int ellipse_set_scalar(GraphicObject* object, const char* key, float value)
{
    EllipseData* ellipse = (EllipseData*)object->impl;
    if (strcmp(key, "x") == 0) { ellipse->bounds.x = value; return 1; }
    if (strcmp(key, "y") == 0) { ellipse->bounds.y = value; return 1; }
    if (strcmp(key, "width") == 0) { ellipse->bounds.w = value; return 1; }
    if (strcmp(key, "height") == 0) { ellipse->bounds.h = value; return 1; }
    return style_set_scalar(object, key, value);
}

/** Line vtable dispatch table. */
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

/** Rectangle vtable dispatch table. */
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

/** Ellipse vtable dispatch table. */
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

/**
 * @brief Allocates a new graphic object.
 * @param type Object type.
 * @param vtable Virtual function table.
 * @param impl Type-specific implementation data.
 * @param style Graphic style.
 * @return New object or NULL.
 */
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

/**
 * @brief Gets the default graphic style.
 * @param void No parameters.
 * @return Default graphic style.
 */
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

/**
 * @brief Gets the type name string.
 * @param type Object type.
 * @return Type name string.
 */
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

/**
 * @brief Creates a line object.
 * @param p1 Start point.
 * @param p2 End point.
 * @param style Graphic style.
 * @return New line object or NULL.
 */
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

/**
 * @brief Creates a rectangle object.
 * @param rect Rectangle bounds.
 * @param style Graphic style.
 * @return New rectangle object or NULL.
 */
GraphicObject* object_create_rect(RectF rect, GraphicStyle style)
{
    RectData* data = (RectData*)calloc(1, sizeof(*data));
    if (!data) {
        return NULL;
    }
    data->rect = rect;
    return object_alloc(GRAPHIC_OBJECT_RECT, &g_rect_vtable, data, style);
}

/**
 * @brief Creates an ellipse object.
 * @param bounds Ellipse bounds.
 * @param style Graphic style.
 * @return New ellipse object or NULL.
 */
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

/**
 * @brief Clones an object by concrete type.
 * @param object Object to clone.
 * @return New object with copied ID/style/revision, or NULL on failure.
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

/**
 * @brief Destroys an object.
 * @param object Object to destroy.
 * @return None.
 */
void object_destroy(GraphicObject* object)
{
    if (object && object->vtable && object->vtable->destroy) {
        object->vtable->destroy(object);
    }
}

/**
 * @brief Translates an object.
 * @param object Object instance.
 * @param delta Translation vector.
 * @return None.
 */
void object_translate(GraphicObject* object, Vec2 delta)
{
    if (!object || !object->vtable || !object->vtable->translate) {
        return;
    }
    object->vtable->translate(object, delta);
    object_bump_revision(object);
}

/**
 * @brief Gets the bounding box of an object.
 * @param object Object instance.
 * @return Bounding rectangle.
 */
RectF object_get_bounds(const GraphicObject* object)
{
    RectF empty = {0.0f, 0.0f, 0.0f, 0.0f};
    if (!object || !object->vtable || !object->vtable->get_bounds) {
        return empty;
    }
    return object->vtable->get_bounds(object);
}

/**
 * @brief Hit tests an object.
 * @param object Object instance.
 * @param point Test point.
 * @param tolerance Hit tolerance.
 * @return 1 if hit, 0 otherwise.
 */
int object_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    if (!object || !object->vtable || !object->vtable->hit_test) {
        return 0;
    }
    return object->vtable->hit_test(object, point, tolerance);
}

/**
 * @brief Gets the path point count.
 * @param object Object instance.
 * @return Number of path points.
 */
int object_get_path_point_count(const GraphicObject* object)
{
    if (!object || !object->vtable || !object->vtable->get_path_point_count) {
        return 0;
    }
    return object->vtable->get_path_point_count(object);
}

/**
 * @brief Writes path points for an object.
 * @param object Object instance.
 * @param out_points Output array for path points.
 * @return None.
 */
void object_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    if (!object || !object->vtable || !object->vtable->write_path_points) {
        return;
    }
    object->vtable->write_path_points(object, out_points);
}

/**
 * @brief Gets a scalar property of an object.
 * @param object Object instance.
 * @param key Property key.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
int object_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    if (!object || !object->vtable || !object->vtable->get_scalar) {
        return 0;
    }
    return object->vtable->get_scalar(object, key, out_value);
}

/**
 * @brief Sets a scalar property of an object.
 * @param object Object instance.
 * @param key Property key.
 * @param value New value.
 * @return 1 on success, 0 on failure.
 */
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

/**
 * @brief Sets the stroke color of an object.
 * @param object Object instance.
 * @param color Stroke color.
 * @return None.
 */
void object_set_stroke_color(GraphicObject* object, Color color)
{
    if (!object) {
        return;
    }
    object->style.stroke_color = color;
    object_bump_revision(object);
}

/**
 * @brief Sets the stroke width of an object.
 * @param object Object instance.
 * @param stroke_width Stroke width.
 * @return None.
 */
void object_set_stroke_width(GraphicObject* object, float stroke_width)
{
    if (!object) {
        return;
    }
    object->style.stroke_width = stroke_width;
    object_bump_revision(object);
}
