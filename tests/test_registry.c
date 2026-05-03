#include <canvas/canvas_view.h>
#include <base/math2d.h>
#include <document/document.h>
#include <document/object.h>
#include <document/persistence.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    RectF bounds;
} FakeStarData;

static int g_failures = 0;

static void expect_true_impl(int condition, const char* expr, const char* file, int line)
{
    if (!condition) {
        fprintf(stderr, "%s:%d: expected true: %s\n", file, line, expr);
        g_failures++;
    }
}

static void expect_int_eq_impl(int actual, int expected, const char* file, int line)
{
    if (actual != expected) {
        fprintf(stderr, "%s:%d: expected %d == %d\n", file, line, actual, expected);
        g_failures++;
    }
}

static void expect_str_eq_impl(const char* actual, const char* expected, const char* file, int line)
{
    if (!actual || !expected || strcmp(actual, expected) != 0) {
        fprintf(stderr,
                "%s:%d: expected \"%s\" == \"%s\"\n",
                file,
                line,
                actual ? actual : "(null)",
                expected ? expected : "(null)");
        g_failures++;
    }
}

#define EXPECT_TRUE(expr) expect_true_impl((expr), #expr, __FILE__, __LINE__)
#define EXPECT_INT_EQ(actual, expected) expect_int_eq_impl((actual), (expected), __FILE__, __LINE__)
#define EXPECT_STR_EQ(actual, expected) expect_str_eq_impl((actual), (expected), __FILE__, __LINE__)

static int fake_star_style_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    if (strcmp(key, "stroke_r") == 0) { *out_value = object->style.stroke_color.r; return 1; }
    if (strcmp(key, "stroke_g") == 0) { *out_value = object->style.stroke_color.g; return 1; }
    if (strcmp(key, "stroke_b") == 0) { *out_value = object->style.stroke_color.b; return 1; }
    if (strcmp(key, "stroke_a") == 0) { *out_value = object->style.stroke_color.a; return 1; }
    if (strcmp(key, "stroke_width") == 0) { *out_value = object->style.stroke_width; return 1; }
    return 0;
}

static int fake_star_style_set_scalar(GraphicObject* object, const char* key, float value)
{
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
    object->descriptor = descriptor;
    object->impl = data;
    object->style = style;
    object->revision = 1u;
    return object;
}

static GraphicObject* fake_star_clone(const GraphicObject* object)
{
    const FakeStarData* data = object ? (const FakeStarData*)object->impl : NULL;
    return data ? fake_star_create(&data->bounds, object->style) : NULL;
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

    for (i = 0; i < 10; ++i) {
        float angle = -1.57079632679f + (float)i * 0.62831853072f;
        float radius = (i % 2 == 0) ? outer_radius : inner_radius;
        out_points[i] = vec2_make(center.x + cosf(angle) * radius,
                                  center.y + sinf(angle) * radius);
    }
    out_points[10] = out_points[0];
}

static int fake_star_get_scalar(const GraphicObject* object, const char* key, float* out_value)
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
        "x", "y", "width", "height", "stroke_r", "stroke_g", "stroke_b", "stroke_a", "stroke_width"
    };
    int i = 0;

    graphic_property_bag_init(out_properties);
    for (i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); ++i) {
        if (object_get_scalar(object, keys[i], &value)) {
            if (!graphic_property_bag_set(out_properties, keys[i], value)) {
                return 0;
            }
        }
    }
    return 1;
}

static GraphicObject* fake_star_deserialize(const GraphicPropertyBag* properties, GraphicStyle style)
{
    RectF bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    GraphicObject* object = NULL;
    float value = 0.0f;

    if (!graphic_property_bag_get(properties, "x", &bounds.x) ||
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
    {"height", GRAPHIC_PROPERTY_FLOAT, 1.0f, 5000.0f, 1.0f, 0.5f}
};

static const GraphicObjectDescriptor g_fake_star_descriptor = {
    100u,
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

static void ensure_fake_star_registered(void)
{
    if (!object_registry_lookup("fake_star")) {
        EXPECT_TRUE(register_object_type(&g_fake_star_descriptor));
    }
}

static int make_temp_path(char* buffer, size_t buffer_size, const char* suffix)
{
    char base_name[L_tmpnam];

    if (!buffer || buffer_size == 0u || !suffix) {
        return 0;
    }

#ifdef _WIN32
    if (tmpnam_s(base_name, sizeof(base_name)) != 0) {
        return 0;
    }
#else
    if (!tmpnam(base_name)) {
        return 0;
    }
#endif

    if (snprintf(buffer, buffer_size, "%s%s", base_name, suffix) >= (int)buffer_size) {
        return 0;
    }

    return 1;
}

static int test_fake_star_registry_round_trip(void)
{
    Document document;
    Document loaded;
    CanvasView canvas;
    GraphicObject* created = NULL;
    GraphicObject* picked = NULL;
    RectF bounds = {-50.0f, -50.0f, 100.0f, 100.0f};
    char path[L_tmpnam + 16];

    ensure_fake_star_registered();

    document_init(&document);
    document_init(&loaded);
    canvas_view_init(&canvas, &document, (RectF){0.0f, 0.0f, 800.0f, 600.0f});

    created = object_create("fake_star", &bounds, object_default_style());
    EXPECT_TRUE(created != NULL);
    EXPECT_TRUE(document_add_object(&document, created));
    EXPECT_STR_EQ(object_type_id(document.objects[0]), "fake_star");
    EXPECT_INT_EQ(object_get_path_point_count(document.objects[0]), 11);

    picked = canvas_view_pick_object(&canvas, vec2_make(400.0f, 300.0f), 4.0f);
    EXPECT_TRUE(picked != NULL);
    EXPECT_STR_EQ(object_type_id(picked), "fake_star");

    EXPECT_TRUE(make_temp_path(path, sizeof(path), ".json"));
    EXPECT_TRUE(document_save_json(&document, path));
    EXPECT_TRUE(document_load_json(&loaded, path));
    EXPECT_INT_EQ(loaded.count, 1);
    EXPECT_STR_EQ(object_type_id(loaded.objects[0]), "fake_star");

    remove(path);
    document_shutdown(&loaded);
    document_shutdown(&document);
    return g_failures == 0;
}

int main(void)
{
    test_fake_star_registry_round_trip();

    if (g_failures != 0) {
        fprintf(stderr, "Test failures: %d\n", g_failures);
        return 1;
    }

    printf("[PASS] object registry fake_star extensibility\n");
    return 0;
}
