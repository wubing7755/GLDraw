/**
 * @file object_line.c
 * @brief Line graphic object type (builtin).
 *
 * Self-contained registration unit following the same pattern as
 * object_fake_star.c.  No other file needs to be modified to add or
 * remove the line type.
 */
#include <document/object.h>

#include <base/math2d.h>

#include "object_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Vec2 p1;
    Vec2 p2;
} LineData;

/* ------------------------------------------------------------------ */
/*  vtable helpers                                                    */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/*  create / clone / destroy                                          */
/* ------------------------------------------------------------------ */

static GraphicObject* line_create(const void* init_data, GraphicStyle style)
{
    const GraphicObjectDescriptor* descriptor = object_registry_lookup("line");
    const Vec2* points = (const Vec2*)init_data;
    LineData* data = NULL;
    GraphicObject* object = NULL;

    if (!descriptor || !points) {
        return NULL;
    }

    data = (LineData*)calloc(1u, sizeof(*data));
    object = (GraphicObject*)calloc(1u, sizeof(*object));
    if (!data || !object) {
        free(data);
        free(object);
        return NULL;
    }

    data->p1 = points[0];
    data->p2 = points[1];
    object->type = descriptor->type;
    object->layer_id = 1u;
    object->descriptor = descriptor;
    object->impl = data;
    object->style = style;
    object->revision = 1u;
    return object;
}

static GraphicObject* line_clone(const GraphicObject* object)
{
    return default_clone_object(object);
}

static void line_destroy(GraphicObject* object)
{
    if (!object) {
        return;
    }
    free(object->impl);
    free(object);
}

/* ------------------------------------------------------------------ */
/*  translate / bounds / hit-test / path-points                       */
/* ------------------------------------------------------------------ */

static void line_translate(GraphicObject* object, Vec2 delta)
{
    LineData* data = object ? (LineData*)object->impl : NULL;
    if (data) {
        data->p1 = vec2_add(data->p1, delta);
        data->p2 = vec2_add(data->p2, delta);
    }
}

static RectF line_get_bounds(const GraphicObject* object)
{
    const LineData* data = object ? (const LineData*)object->impl : NULL;
    RectF empty = {0.0f, 0.0f, 0.0f, 0.0f};

    if (!data) {
        return empty;
    }

    {
        float min_x = (data->p1.x < data->p2.x) ? data->p1.x : data->p2.x;
        float min_y = (data->p1.y < data->p2.y) ? data->p1.y : data->p2.y;
        float max_x = (data->p1.x > data->p2.x) ? data->p1.x : data->p2.x;
        float max_y = (data->p1.y > data->p2.y) ? data->p1.y : data->p2.y;
        RectF bounds = {min_x, min_y, max_x - min_x, max_y - min_y};
        return bounds;
    }
}

static int line_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    const LineData* data = object ? (const LineData*)object->impl : NULL;
    if (!data) {
        return 0;
    }
    return line_distance_to_segment(point, data->p1, data->p2) <= tolerance;
}

static int line_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return 2;
}

static void line_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const LineData* data = object ? (const LineData*)object->impl : NULL;
    if (!data || !out_points) {
        return;
    }
    out_points[0] = data->p1;
    out_points[1] = data->p2;
}

/* ------------------------------------------------------------------ */
/*  get_scalar / set_scalar                                           */
/* ------------------------------------------------------------------ */

static int line_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const LineData* data = object ? (const LineData*)object->impl : NULL;

    if (!data || !key || !out_value) {
        return 0;
    }

    if (strcmp(key, "x1") == 0) { *out_value = data->p1.x; return 1; }
    if (strcmp(key, "y1") == 0) { *out_value = data->p1.y; return 1; }
    if (strcmp(key, "x2") == 0) { *out_value = data->p2.x; return 1; }
    if (strcmp(key, "y2") == 0) { *out_value = data->p2.y; return 1; }
    return style_get_scalar(object, key, out_value);
}

static int line_set_scalar(GraphicObject* object, const char* key, float value)
{
    LineData* data = object ? (LineData*)object->impl : NULL;

    if (!data || !key) {
        return 0;
    }

    if (strcmp(key, "x1") == 0) { data->p1.x = value; return 1; }
    if (strcmp(key, "y1") == 0) { data->p1.y = value; return 1; }
    if (strcmp(key, "x2") == 0) { data->p2.x = value; return 1; }
    if (strcmp(key, "y2") == 0) { data->p2.y = value; return 1; }
    return style_set_scalar(object, key, value);
}

/* ------------------------------------------------------------------ */
/*  serialize / deserialize                                           */
/* ------------------------------------------------------------------ */

static int line_serialize(const GraphicObject* object, GraphicPropertyBag* out_properties)
{
    return default_serialize_from_schema(object, out_properties);
}

static GraphicObject* line_deserialize(const GraphicPropertyBag* properties,
                                       GraphicStyle style)
{
    Vec2 points[2] = {{0.0f, 0.0f}, {0.0f, 0.0f}};
    GraphicObject* object = NULL;
    float value = 0.0f;

    if (!properties ||
        !graphic_property_bag_get(properties, "x1", &points[0].x) ||
        !graphic_property_bag_get(properties, "y1", &points[0].y) ||
        !graphic_property_bag_get(properties, "x2", &points[1].x) ||
        !graphic_property_bag_get(properties, "y2", &points[1].y)) {
        return NULL;
    }

    object = line_create(points, style);
    if (!object) {
        return NULL;
    }

    if (graphic_property_bag_get(properties, "stroke_r", &value)) { object->style.stroke_color.r = value; }
    if (graphic_property_bag_get(properties, "stroke_g", &value)) { object->style.stroke_color.g = value; }
    if (graphic_property_bag_get(properties, "stroke_b", &value)) { object->style.stroke_color.b = value; }
    if (graphic_property_bag_get(properties, "stroke_a", &value)) { object->style.stroke_color.a = value; }
    if (graphic_property_bag_get(properties, "stroke_width", &value) && value > 0.0f) {
        object->style.stroke_width = value;
    }
    return object;
}

/* ------------------------------------------------------------------ */
/*  schema & descriptor                                               */
/* ------------------------------------------------------------------ */

static const GraphicPropertyDef g_line_schema[] = {
    {"x1", GRAPHIC_PROPERTY_FLOAT, -5000.0f, 5000.0f, 1.0f, 0.5f},
    {"y1", GRAPHIC_PROPERTY_FLOAT, -5000.0f, 5000.0f, 1.0f, 0.5f},
    {"x2", GRAPHIC_PROPERTY_FLOAT, -5000.0f, 5000.0f, 1.0f, 0.5f},
    {"y2", GRAPHIC_PROPERTY_FLOAT, -5000.0f, 5000.0f, 1.0f, 0.5f},
    {"stroke_r", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_g", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_b", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_a", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_width", GRAPHIC_PROPERTY_FLOAT, 1.0f, 12.0f, 0.1f, 0.1f}
};

static const GraphicObjectDescriptor g_line_descriptor = {
    GRAPHIC_OBJECT_INVALID, /* auto-assigned by registry */
    "line",
    "Line",
    line_create,
    line_clone,
    line_destroy,
    line_translate,
    line_get_bounds,
    line_hit_test,
    line_get_path_point_count,
    line_write_path_points,
    line_get_scalar,
    line_set_scalar,
    line_serialize,
    line_deserialize,
    g_line_schema,
    (int)(sizeof(g_line_schema) / sizeof(g_line_schema[0]))
};

int register_line_object_type(void)
{
    if (object_registry_lookup("line")) {
        return 1;
    }
    return register_object_type(&g_line_descriptor);
}

GraphicObject* object_create_line(Vec2 p1, Vec2 p2, GraphicStyle style)
{
    Vec2 points[2] = {p1, p2};
    return object_create("line", points, style);
}
