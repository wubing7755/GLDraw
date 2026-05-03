#include <document/object.h>

#include <base/math2d.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    RectF bounds;
} FakeStarData;

static int fake_star_style_get_scalar(const GraphicObject* object,
                                      const char* key,
                                      float* out_value)
{
    if (!object || !key || !out_value) {
        return 0;
    }

    if (strcmp(key, "stroke_r") == 0) { *out_value = object->style.stroke_color.r; return 1; }
    if (strcmp(key, "stroke_g") == 0) { *out_value = object->style.stroke_color.g; return 1; }
    if (strcmp(key, "stroke_b") == 0) { *out_value = object->style.stroke_color.b; return 1; }
    if (strcmp(key, "stroke_a") == 0) { *out_value = object->style.stroke_color.a; return 1; }
    if (strcmp(key, "stroke_width") == 0) { *out_value = object->style.stroke_width; return 1; }
    return 0;
}

static int fake_star_style_set_scalar(GraphicObject* object,
                                      const char* key,
                                      float value)
{
    if (!object || !key) {
        return 0;
    }

    if (strcmp(key, "stroke_r") == 0) { object->style.stroke_color.r = value; return 1; }
    if (strcmp(key, "stroke_g") == 0) { object->style.stroke_color.g = value; return 1; }
    if (strcmp(key, "stroke_b") == 0) { object->style.stroke_color.b = value; return 1; }
    if (strcmp(key, "stroke_a") == 0) { object->style.stroke_color.a = value; return 1; }
    if (strcmp(key, "stroke_width") == 0) { object->style.stroke_width = value; return 1; }
    return 0;
}

static GraphicObject* fake_star_create(const void* init_data, GraphicStyle style)
{
    const GraphicObjectDescriptor* descriptor = object_registry_lookup("fake_star");
    const RectF* bounds = (const RectF*)init_data;
    FakeStarData* data = NULL;
    GraphicObject* object = NULL;

    if (!descriptor || !bounds || bounds->w <= 0.0f || bounds->h <= 0.0f) {
        return NULL;
    }

    data = (FakeStarData*)calloc(1u, sizeof(*data));
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

static GraphicObject* fake_star_clone(const GraphicObject* object)
{
    const FakeStarData* data = object ? (const FakeStarData*)object->impl : NULL;
    GraphicObject* clone = data ? fake_star_create(&data->bounds, object->style) : NULL;

    if (clone && object) {
        clone->id = object->id;
        clone->layer_id = object->layer_id;
        clone->revision = object->revision;
    }
    return clone;
}

static void fake_star_destroy(GraphicObject* object)
{
    if (!object) {
        return;
    }

    free(object->impl);
    free(object);
}

static void fake_star_translate(GraphicObject* object, Vec2 delta)
{
    FakeStarData* data = object ? (FakeStarData*)object->impl : NULL;
    if (data) {
        data->bounds.x += delta.x;
        data->bounds.y += delta.y;
    }
}

static RectF fake_star_get_bounds(const GraphicObject* object)
{
    const FakeStarData* data = object ? (const FakeStarData*)object->impl : NULL;
    return data ? data->bounds : (RectF){0.0f, 0.0f, 0.0f, 0.0f};
}

static int fake_star_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    RectF bounds = fake_star_get_bounds(object);
    bounds.x -= tolerance;
    bounds.y -= tolerance;
    bounds.w += tolerance * 2.0f;
    bounds.h += tolerance * 2.0f;
    return rectf_contains_point(&bounds, point);
}

static int fake_star_get_path_point_count(const GraphicObject* object)
{
    (void)object;
    return 11;
}

static void fake_star_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    RectF bounds = fake_star_get_bounds(object);
    Vec2 center = vec2_make(bounds.x + bounds.w * 0.5f, bounds.y + bounds.h * 0.5f);
    float outer_radius = (bounds.w < bounds.h ? bounds.w : bounds.h) * 0.5f;
    float inner_radius = outer_radius * 0.45f;
    int i = 0;

    if (!out_points) {
        return;
    }

    for (i = 0; i < 10; ++i) {
        float angle = -1.57079632679f + (float)i * 0.62831853072f;
        float radius = (i % 2 == 0) ? outer_radius : inner_radius;
        out_points[i] = vec2_make(center.x + cosf(angle) * radius,
                                  center.y + sinf(angle) * radius);
    }
    out_points[10] = out_points[0];
}

static int fake_star_get_scalar(const GraphicObject* object,
                                const char* key,
                                float* out_value)
{
    const FakeStarData* data = object ? (const FakeStarData*)object->impl : NULL;

    if (!data || !key || !out_value) {
        return 0;
    }

    if (strcmp(key, "x") == 0) { *out_value = data->bounds.x; return 1; }
    if (strcmp(key, "y") == 0) { *out_value = data->bounds.y; return 1; }
    if (strcmp(key, "width") == 0) { *out_value = data->bounds.w; return 1; }
    if (strcmp(key, "height") == 0) { *out_value = data->bounds.h; return 1; }
    return fake_star_style_get_scalar(object, key, out_value);
}

static int fake_star_set_scalar(GraphicObject* object, const char* key, float value)
{
    FakeStarData* data = object ? (FakeStarData*)object->impl : NULL;

    if (!data || !key) {
        return 0;
    }

    if (strcmp(key, "x") == 0) { data->bounds.x = value; return 1; }
    if (strcmp(key, "y") == 0) { data->bounds.y = value; return 1; }
    if (strcmp(key, "width") == 0 && value > 0.0f) { data->bounds.w = value; return 1; }
    if (strcmp(key, "height") == 0 && value > 0.0f) { data->bounds.h = value; return 1; }
    return fake_star_style_set_scalar(object, key, value);
}

static int fake_star_serialize(const GraphicObject* object, GraphicPropertyBag* out_properties)
{
    float value = 0.0f;
    static const char* keys[] = {
        "x", "y", "width", "height",
        "stroke_r", "stroke_g", "stroke_b", "stroke_a", "stroke_width"
    };
    int i = 0;

    if (!object || !out_properties) {
        return 0;
    }

    graphic_property_bag_init(out_properties);
    for (i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); ++i) {
        if (object_get_scalar(object, keys[i], &value) &&
            !graphic_property_bag_set(out_properties, keys[i], value)) {
            return 0;
        }
    }
    return 1;
}

static GraphicObject* fake_star_deserialize(const GraphicPropertyBag* properties,
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

    object = fake_star_create(&bounds, style);
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

static const GraphicPropertyDef g_fake_star_schema[] = {
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

static const GraphicObjectDescriptor g_fake_star_descriptor = {
    GRAPHIC_OBJECT_INVALID, /* auto-assigned by registry */
    "fake_star",
    "Fake Star",
    fake_star_create,
    fake_star_clone,
    fake_star_destroy,
    fake_star_translate,
    fake_star_get_bounds,
    fake_star_hit_test,
    fake_star_get_path_point_count,
    fake_star_write_path_points,
    fake_star_get_scalar,
    fake_star_set_scalar,
    fake_star_serialize,
    fake_star_deserialize,
    g_fake_star_schema,
    (int)(sizeof(g_fake_star_schema) / sizeof(g_fake_star_schema[0]))
};

int register_fake_star_object_extension(void)
{
    if (object_registry_lookup("fake_star")) {
        return 1;
    }

    return register_object_type(&g_fake_star_descriptor);
}
