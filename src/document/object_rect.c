/**
 * @file object_rect.c
 * @brief Rectangle graphic object type (builtin).
 *
 * Self-contained registration unit following the same pattern as
 * object_fake_star.c.  No other file needs to be modified to add or
 * remove the rectangle type.
 */
#include <document/object.h>

#include <base/math2d.h>

#include "object_internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    RectF rect;
} RectData;

/* ------------------------------------------------------------------ */
/*  create / clone / destroy                                          */
/* ------------------------------------------------------------------ */

static GraphicObject* rect_create(const void* init_data, GraphicStyle style)
{
    const GraphicObjectDescriptor* descriptor = object_registry_lookup("rect");
    const RectF* bounds = (const RectF*)init_data;
    RectData* data = NULL;
    GraphicObject* object = NULL;

    if (!descriptor || !bounds) {
        return NULL;
    }

    data = (RectData*)calloc(1u, sizeof(*data));
    object = (GraphicObject*)calloc(1u, sizeof(*object));
    if (!data || !object) {
        free(data);
        free(object);
        return NULL;
    }

    data->rect = *bounds;
    object->type = descriptor->type;
    object->layer_id = 1u;
    object->descriptor = descriptor;
    object->impl = data;
    object->style = style;
    object->revision = 1u;
    return object;
}

static GraphicObject* rect_clone(const GraphicObject* object)
{
    return default_clone_object(object);
}

static void rect_destroy(GraphicObject* object)
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

static void rect_translate(GraphicObject* object, Vec2 delta)
{
    RectData* data = object ? (RectData*)object->impl : NULL;
    if (data) {
        data->rect.x += delta.x;
        data->rect.y += delta.y;
    }
}

static RectF rect_get_bounds(const GraphicObject* object)
{
    const RectData* data = object ? (const RectData*)object->impl : NULL;
    return data ? data->rect : (RectF){0.0f, 0.0f, 0.0f, 0.0f};
}

static int rect_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    RectF bounds = rect_get_bounds(object);
    bounds.x -= tolerance;
    bounds.y -= tolerance;
    bounds.w += tolerance * 2.0f;
    bounds.h += tolerance * 2.0f;
    return rectf_contains_point(&bounds, point);
}

static int rect_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return 5;
}

static void rect_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    const RectData* data = object ? (const RectData*)object->impl : NULL;
    if (!data || !out_points) {
        return;
    }
    out_points[0] = vec2_make(data->rect.x, data->rect.y);
    out_points[1] = vec2_make(data->rect.x + data->rect.w, data->rect.y);
    out_points[2] = vec2_make(data->rect.x + data->rect.w, data->rect.y + data->rect.h);
    out_points[3] = vec2_make(data->rect.x, data->rect.y + data->rect.h);
    out_points[4] = out_points[0];
}

/* ------------------------------------------------------------------ */
/*  get_scalar / set_scalar                                           */
/* ------------------------------------------------------------------ */

static int rect_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    const RectData* data = object ? (const RectData*)object->impl : NULL;

    if (!data || !key || !out_value) {
        return 0;
    }

    if (strcmp(key, "x") == 0) { *out_value = data->rect.x; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = data->rect.y; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = data->rect.w; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = data->rect.h; return 1; }
    return style_get_scalar(object, key, out_value);
}

static int rect_set_scalar(GraphicObject* object, const char* key, float value)
{
    RectData* data = object ? (RectData*)object->impl : NULL;

    if (!data || !key) {
        return 0;
    }

    if (strcmp(key, "x") == 0) { data->rect.x = value; return 1; }
    if (strcmp(key, "y") == 0) { data->rect.y = value; return 1; }
    if (strcmp(key, "width") == 0) { data->rect.w = value; return 1; }
    if (strcmp(key, "height") == 0) { data->rect.h = value; return 1; }
    return style_set_scalar(object, key, value);
}

/* ------------------------------------------------------------------ */
/*  serialize / deserialize                                           */
/* ------------------------------------------------------------------ */

static int rect_serialize(const GraphicObject* object, GraphicPropertyBag* out_properties)
{
    return default_serialize_from_schema(object, out_properties);
}

static GraphicObject* rect_deserialize(const GraphicPropertyBag* properties,
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

    object = rect_create(&bounds, style);
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

static const GraphicPropertyDef g_rect_schema[] = {
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

static const GraphicObjectDescriptor g_rect_descriptor = {
    GRAPHIC_OBJECT_INVALID, /* auto-assigned by registry */
    "rect",
    "Rectangle",
    rect_create,
    rect_clone,
    rect_destroy,
    rect_translate,
    rect_get_bounds,
    rect_hit_test,
    rect_get_path_point_count,
    rect_write_path_points,
    rect_get_scalar,
    rect_set_scalar,
    rect_serialize,
    rect_deserialize,
    g_rect_schema,
    (int)(sizeof(g_rect_schema) / sizeof(g_rect_schema[0]))
};

int register_rect_object_type(void)
{
    if (object_registry_lookup("rect")) {
        return 1;
    }
    return register_object_type(&g_rect_descriptor);
}

GraphicObject* object_create_rect(RectF rect, GraphicStyle style)
{
    return object_create("rect", &rect, style);
}
