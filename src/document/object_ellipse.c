/**
 * @file object_ellipse.c
 * @brief Ellipse graphic object type (builtin).
 *
 * Self-contained registration unit following the same pattern as
 * object_fake_star.c.  No other file needs to be modified to add or
 * remove the ellipse type.
 */
#include <document/object.h>

#include <base/math2d.h>

#include "object_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ELLIPSE_SEGMENTS 64

typedef struct {
    RectF bounds;
} EllipseData;

/* ------------------------------------------------------------------ */
/*  create / clone / destroy                                          */
/* ------------------------------------------------------------------ */

static GraphicObject* ellipse_create(const void* init_data, GraphicStyle style)
{
    const GraphicObjectDescriptor* descriptor = object_registry_lookup("ellipse");
    const RectF* bounds = (const RectF*)init_data;
    EllipseData* data = NULL;
    GraphicObject* object = NULL;

    if (!descriptor || !bounds) {
        return NULL;
    }

    data = (EllipseData*)calloc(1u, sizeof(*data));
    object = (GraphicObject*)calloc(1u, sizeof(*object));
    if (!data || !object) {
        free(data);
        free(object);
        return NULL;
    }

    data->bounds = *bounds;
    object->type = descriptor->type;
    object->layer_id = 1u;
    object->descriptor = descriptor;
    object->impl = data;
    object->style = style;
    object->revision = 1u;
    return object;
}

static GraphicObject* ellipse_clone(const GraphicObject* object)
{
    return default_clone_object(object);
}

static void ellipse_destroy(GraphicObject* object)
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

static void ellipse_translate(GraphicObject* object, Vec2 delta)
{
    EllipseData* data = object ? (EllipseData*)object->impl : NULL;
    if (data) {
        data->bounds.x += delta.x;
        data->bounds.y += delta.y;
    }
}

static RectF ellipse_get_bounds(const GraphicObject* object)
{
    const EllipseData* data = object ? (const EllipseData*)object->impl : NULL;
    return data ? data->bounds : (RectF){0.0f, 0.0f, 0.0f, 0.0f};
}

static int ellipse_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    RectF bounds = ellipse_get_bounds(object);
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

static int ellipse_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return ELLIPSE_SEGMENTS + 1;
}

static void ellipse_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const EllipseData* data = object ? (const EllipseData*)object->impl : NULL;
    float rx = 0.0f;
    float ry = 0.0f;
    Vec2 center = {0.0f, 0.0f};
    int i = 0;

    if (!data || !out_points) {
        return;
    }

    rx = data->bounds.w * 0.5f;
    ry = data->bounds.h * 0.5f;
    center = vec2_make(data->bounds.x + rx, data->bounds.y + ry);

    for (i = 0; i < ELLIPSE_SEGMENTS; ++i) {
        float t = (float)i / (float)ELLIPSE_SEGMENTS;
        float angle = t * 6.28318530718f;
        out_points[i] = vec2_make(center.x + cosf(angle) * rx,
                                  center.y + sinf(angle) * ry);
    }
    out_points[ELLIPSE_SEGMENTS] = out_points[0];
}

/* ------------------------------------------------------------------ */
/*  get_scalar / set_scalar                                           */
/* ------------------------------------------------------------------ */

static int ellipse_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const EllipseData* data = object ? (const EllipseData*)object->impl : NULL;

    if (!data || !key || !out_value) {
        return 0;
    }

    if (strcmp(key, "x") == 0) { *out_value = data->bounds.x; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = data->bounds.y; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = data->bounds.w; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = data->bounds.h; return 1; }
    return style_get_scalar(object, key, out_value);
}

static int ellipse_set_scalar(GraphicObject* object, const char* key, float value)
{
    EllipseData* data = object ? (EllipseData*)object->impl : NULL;

    if (!data || !key) {
        return 0;
    }

    if (strcmp(key, "x") == 0) { data->bounds.x = value; return 1; }
    if (strcmp(key, "y") == 0) { data->bounds.y = value; return 1; }
    if (strcmp(key, "width") == 0) { data->bounds.w = value; return 1; }
    if (strcmp(key, "height") == 0) { data->bounds.h = value; return 1; }
    return style_set_scalar(object, key, value);
}

/* ------------------------------------------------------------------ */
/*  serialize / deserialize                                           */
/* ------------------------------------------------------------------ */

static int ellipse_serialize(const GraphicObject* object, GraphicPropertyBag* out_properties)
{
    return default_serialize_from_schema(object, out_properties);
}

static GraphicObject* ellipse_deserialize(const GraphicPropertyBag* properties,
                                           GraphicStyle style)
{
    RectF bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    GraphicObject* object = NULL;
    float value = 0.0f;

    if (!properties ||
        !graphic_property_bag_get(properties, "x", &bounds.x) ||
        !graphic_property_bag_get(properties, "y", &bounds.y) ||
        !graphic_property_bag_get(properties, "width", &bounds.w) ||
        !graphic_property_bag_get(properties, "height", &bounds.h) ||
        bounds.w <= 0.0f || bounds.h <= 0.0f) {
        return NULL;
    }

    object = ellipse_create(&bounds, style);
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

static const GraphicPropertyDef g_ellipse_schema[] = {
    {"x", GRAPHIC_PROPERTY_FLOAT, -5000.0f, 5000.0f, 1.0f, 0.5f},
    {"y", GRAPHIC_PROPERTY_FLOAT, -5000.0f, 5000.0f, 1.0f, 0.5f},
    {"width", GRAPHIC_PROPERTY_FLOAT, 1.0f, 5000.0f, 1.0f, 0.5f},
    {"height", GRAPHIC_PROPERTY_FLOAT, 1.0f, 5000.0f, 1.0f, 0.5f},
    {"stroke_r", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_g", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_b", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_a", GRAPHIC_PROPERTY_FLOAT, 0.0f, 1.0f, 0.01f, 0.01f},
    {"stroke_width", GRAPHIC_PROPERTY_FLOAT, 1.0f, 12.0f, 0.1f, 0.1f}
};

static const GraphicObjectDescriptor g_ellipse_descriptor = {
    GRAPHIC_OBJECT_INVALID, /* auto-assigned by registry */
    "ellipse",
    "Ellipse",
    ellipse_create,
    ellipse_clone,
    ellipse_destroy,
    ellipse_translate,
    ellipse_get_bounds,
    ellipse_hit_test,
    ellipse_get_path_point_count,
    ellipse_write_path_points,
    ellipse_get_scalar,
    ellipse_set_scalar,
    ellipse_serialize,
    ellipse_deserialize,
    g_ellipse_schema,
    (int)(sizeof(g_ellipse_schema) / sizeof(g_ellipse_schema[0]))
};

int register_ellipse_object_type(void)
{
    if (object_registry_lookup("ellipse")) {
        return 1;
    }
    return register_object_type(&g_ellipse_descriptor);
}

GraphicObject* object_create_ellipse(RectF bounds, GraphicStyle style)
{
    return object_create("ellipse", &bounds, style);
}
