#include <document/object.h>

#include <base/math2d.h>

#include <math.h>
#include <stdio.h>
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

typedef struct {
    GraphicObjectDescriptor descriptors[GRAPHIC_OBJECT_MAX_TYPES];
    int count;
    unsigned int next_dynamic_type;
    int initialized;
} ObjectRegistryState;

static ObjectRegistryState g_registry = {0};

static void ensure_registry_builtins(void);
void object_register_builtin_extensions(void);

static void object_bump_revision(GraphicObject* object)
{
    if (object) {
        object->revision++;
    }
}

static int property_def_is_valid(const GraphicPropertyDef* def)
{
    return def && def->name && def->name[0] != '\0';
}

static int style_get_scalar(const GraphicObject* object, const char* key, float* out_value)
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

static int style_set_scalar(GraphicObject* object, const char* key, float value)
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

static GraphicObject* object_alloc(GraphicObjectType type,
                                   const GraphicObjectDescriptor* descriptor,
                                   void* impl,
                                   GraphicStyle style)
{
    GraphicObject* object = (GraphicObject*)calloc(1, sizeof(*object));
    if (!object) {
        free(impl);
        return NULL;
    }

    object->type = type;
    object->descriptor = descriptor;
    object->impl = impl;
    object->style = style;
    object->layer_id = 1u;
    object->revision = 1u;
    return object;
}

static GraphicObjectDescriptor* object_registry_alloc_slot(void)
{
    if (g_registry.count >= GRAPHIC_OBJECT_MAX_TYPES) {
        return NULL;
    }

    return &g_registry.descriptors[g_registry.count++];
}

static const GraphicObjectDescriptor* object_registry_find_by_type_id(const char* type_id)
{
    int i = 0;

    if (!type_id || type_id[0] == '\0') {
        return NULL;
    }

    for (i = 0; i < g_registry.count; ++i) {
        if (g_registry.descriptors[i].type_id &&
            strcmp(g_registry.descriptors[i].type_id, type_id) == 0) {
            return &g_registry.descriptors[i];
        }
    }

    return NULL;
}

static const GraphicObjectDescriptor* object_registry_find_by_type(GraphicObjectType type)
{
    int i = 0;

    for (i = 0; i < g_registry.count; ++i) {
        if (g_registry.descriptors[i].type == type) {
            return &g_registry.descriptors[i];
        }
    }

    return NULL;
}

void object_registry_init(void)
{
    if (g_registry.initialized) {
        return;
    }

    memset(&g_registry, 0, sizeof(g_registry));
    g_registry.initialized = 1;
    g_registry.next_dynamic_type = GRAPHIC_OBJECT_ELLIPSE + 1u;
}

int register_object_type(const GraphicObjectDescriptor* descriptor)
{
    GraphicObjectDescriptor* slot = NULL;
    GraphicObjectDescriptor copy;

    if (!descriptor || !descriptor->type_id || descriptor->type_id[0] == '\0' ||
        !descriptor->name || descriptor->name[0] == '\0' ||
        !descriptor->create || !descriptor->clone || !descriptor->destroy ||
        !descriptor->get_bounds || !descriptor->hit_test ||
        !descriptor->get_path_point_count || !descriptor->write_path_points ||
        !descriptor->get_scalar || !descriptor->set_scalar ||
        !descriptor->serialize || !descriptor->deserialize) {
        return 0;
    }

    object_registry_init();
    if (object_registry_find_by_type_id(descriptor->type_id) ||
        object_registry_find_by_type(descriptor->type)) {
        return 0;
    }

    copy = *descriptor;
    if (copy.type == GRAPHIC_OBJECT_INVALID) {
        copy.type = g_registry.next_dynamic_type++;
    } else if (copy.type >= g_registry.next_dynamic_type) {
        g_registry.next_dynamic_type = copy.type + 1u;
    }

    slot = object_registry_alloc_slot();
    if (!slot) {
        return 0;
    }

    *slot = copy;
    return 1;
}

const GraphicObjectDescriptor* object_registry_lookup(const char* type_id)
{
    ensure_registry_builtins();
    return object_registry_find_by_type_id(type_id);
}

const GraphicObjectDescriptor* object_registry_lookup_by_type(GraphicObjectType type)
{
    ensure_registry_builtins();
    return object_registry_find_by_type(type);
}

int object_registry_count(void)
{
    ensure_registry_builtins();
    return g_registry.count;
}

const GraphicObjectDescriptor* object_registry_at(int index)
{
    ensure_registry_builtins();

    if (index < 0 || index >= g_registry.count) {
        return NULL;
    }

    return &g_registry.descriptors[index];
}

void graphic_property_bag_init(GraphicPropertyBag* bag)
{
    if (bag) {
        memset(bag, 0, sizeof(*bag));
    }
}

int graphic_property_bag_set(GraphicPropertyBag* bag, const char* name, float value)
{
    int i = 0;

    if (!bag || !name || name[0] == '\0') {
        return 0;
    }

    for (i = 0; i < bag->count; ++i) {
        if (strcmp(bag->values[i].name, name) == 0) {
            bag->values[i].value = value;
            return 1;
        }
    }

    if (bag->count >= GRAPHIC_OBJECT_MAX_PROPERTIES) {
        return 0;
    }

    snprintf(bag->values[bag->count].name,
             sizeof(bag->values[bag->count].name),
             "%s",
             name);
    bag->values[bag->count].value = value;
    bag->count++;
    return 1;
}

int graphic_property_bag_get(const GraphicPropertyBag* bag, const char* name, float* out_value)
{
    int i = 0;

    if (!bag || !name || !out_value) {
        return 0;
    }

    for (i = 0; i < bag->count; ++i) {
        if (strcmp(bag->values[i].name, name) == 0) {
            *out_value = bag->values[i].value;
            return 1;
        }
    }

    return 0;
}

static int default_serialize_from_schema(const GraphicObject* object, GraphicPropertyBag* out_properties)
{
    const GraphicPropertyDef* schema = NULL;
    int count = 0;
    int i = 0;

    if (!object || !out_properties || !object->descriptor) {
        return 0;
    }

    graphic_property_bag_init(out_properties);
    schema = object->descriptor->property_schema;
    count = object->descriptor->property_count;

    for (i = 0; i < count; ++i) {
        float value = 0.0f;

        if (!property_def_is_valid(&schema[i])) {
            continue;
        }
        if (!object->descriptor->get_scalar(object, schema[i].name, &value) &&
            !style_get_scalar(object, schema[i].name, &value)) {
            continue;
        }
        if (!graphic_property_bag_set(out_properties, schema[i].name, value)) {
            return 0;
        }
    }

    return 1;
}

static GraphicObject* default_clone_object(const GraphicObject* object)
{
    GraphicPropertyBag properties;

    if (!object || !object->descriptor || !object->descriptor->serialize ||
        !object->descriptor->deserialize) {
        return NULL;
    }

    graphic_property_bag_init(&properties);
    if (!object->descriptor->serialize(object, &properties)) {
        return NULL;
    }

    return object->descriptor->deserialize(&properties, object->style);
}

static int builtin_object_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    if (!object || !object->impl) {
        return 0;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        const LineData* line = (const LineData*)object->impl;
        if (strcmp(key, "x1") == 0) { *out_value = line->p1.x; return 1; }
        if (strcmp(key, "y1") == 0) { *out_value = line->p1.y; return 1; }
        if (strcmp(key, "x2") == 0) { *out_value = line->p2.x; return 1; }
        if (strcmp(key, "y2") == 0) { *out_value = line->p2.y; return 1; }
    } else if (object->type == GRAPHIC_OBJECT_RECT) {
        const RectData* rect = (const RectData*)object->impl;
        if (strcmp(key, "x") == 0) { *out_value = rect->rect.x; return 1; }
        if (strcmp(key, "y") == 0) { *out_value = rect->rect.y; return 1; }
        if (strcmp(key, "width") == 0) { *out_value = rect->rect.w; return 1; }
        if (strcmp(key, "height") == 0) { *out_value = rect->rect.h; return 1; }
    } else if (object->type == GRAPHIC_OBJECT_ELLIPSE) {
        const EllipseData* ellipse = (const EllipseData*)object->impl;
        if (strcmp(key, "x") == 0) { *out_value = ellipse->bounds.x; return 1; }
        if (strcmp(key, "y") == 0) { *out_value = ellipse->bounds.y; return 1; }
        if (strcmp(key, "width") == 0) { *out_value = ellipse->bounds.w; return 1; }
        if (strcmp(key, "height") == 0) { *out_value = ellipse->bounds.h; return 1; }
    }

    return style_get_scalar(object, key, out_value);
}

static int builtin_object_set_scalar(GraphicObject* object, const char* key, float value)
{
    if (!object || !object->impl) {
        return 0;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        LineData* line = (LineData*)object->impl;
        if (strcmp(key, "x1") == 0) { line->p1.x = value; return 1; }
        if (strcmp(key, "y1") == 0) { line->p1.y = value; return 1; }
        if (strcmp(key, "x2") == 0) { line->p2.x = value; return 1; }
        if (strcmp(key, "y2") == 0) { line->p2.y = value; return 1; }
    } else if (object->type == GRAPHIC_OBJECT_RECT) {
        RectData* rect = (RectData*)object->impl;
        if (strcmp(key, "x") == 0) { rect->rect.x = value; return 1; }
        if (strcmp(key, "y") == 0) { rect->rect.y = value; return 1; }
        if (strcmp(key, "width") == 0) { rect->rect.w = value; return 1; }
        if (strcmp(key, "height") == 0) { rect->rect.h = value; return 1; }
    } else if (object->type == GRAPHIC_OBJECT_ELLIPSE) {
        EllipseData* ellipse = (EllipseData*)object->impl;
        if (strcmp(key, "x") == 0) { ellipse->bounds.x = value; return 1; }
        if (strcmp(key, "y") == 0) { ellipse->bounds.y = value; return 1; }
        if (strcmp(key, "width") == 0) { ellipse->bounds.w = value; return 1; }
        if (strcmp(key, "height") == 0) { ellipse->bounds.h = value; return 1; }
    }

    return style_set_scalar(object, key, value);
}

static void builtin_object_destroy(GraphicObject* object)
{
    if (!object) {
        return;
    }

    free(object->impl);
    free(object);
}

static void builtin_object_translate(GraphicObject* object, Vec2 delta)
{
    if (!object || !object->impl) {
        return;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        LineData* line = (LineData*)object->impl;
        line->p1 = vec2_add(line->p1, delta);
        line->p2 = vec2_add(line->p2, delta);
    } else if (object->type == GRAPHIC_OBJECT_RECT) {
        RectData* rect = (RectData*)object->impl;
        rect->rect.x += delta.x;
        rect->rect.y += delta.y;
    } else if (object->type == GRAPHIC_OBJECT_ELLIPSE) {
        EllipseData* ellipse = (EllipseData*)object->impl;
        ellipse->bounds.x += delta.x;
        ellipse->bounds.y += delta.y;
    }
}

static RectF builtin_object_get_bounds(const GraphicObject* object)
{
    RectF empty = {0.0f, 0.0f, 0.0f, 0.0f};

    if (!object || !object->impl) {
        return empty;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        const LineData* line = (const LineData*)object->impl;
        float min_x = (line->p1.x < line->p2.x) ? line->p1.x : line->p2.x;
        float min_y = (line->p1.y < line->p2.y) ? line->p1.y : line->p2.y;
        float max_x = (line->p1.x > line->p2.x) ? line->p1.x : line->p2.x;
        float max_y = (line->p1.y > line->p2.y) ? line->p1.y : line->p2.y;
        RectF bounds = {min_x, min_y, max_x - min_x, max_y - min_y};
        return bounds;
    }

    if (object->type == GRAPHIC_OBJECT_RECT) {
        const RectData* rect = (const RectData*)object->impl;
        return rect->rect;
    }

    if (object->type == GRAPHIC_OBJECT_ELLIPSE) {
        const EllipseData* ellipse = (const EllipseData*)object->impl;
        return ellipse->bounds;
    }

    return empty;
}

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

static int builtin_object_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    if (!object || !object->impl) {
        return 0;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        const LineData* line = (const LineData*)object->impl;
        return line_distance_to_segment(point, line->p1, line->p2) <= tolerance;
    }

    if (object->type == GRAPHIC_OBJECT_RECT) {
        RectF bounds = builtin_object_get_bounds(object);
        bounds.x -= tolerance;
        bounds.y -= tolerance;
        bounds.w += tolerance * 2.0f;
        bounds.h += tolerance * 2.0f;
        return rectf_contains_point(&bounds, point);
    }

    if (object->type == GRAPHIC_OBJECT_ELLIPSE) {
        RectF bounds = builtin_object_get_bounds(object);
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

    return 0;
}

static int builtin_object_get_path_point_count(const GraphicObject* object)
{
    if (!object) {
        return 0;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        return 2;
    }
    if (object->type == GRAPHIC_OBJECT_RECT) {
        return 5;
    }
    if (object->type == GRAPHIC_OBJECT_ELLIPSE) {
        return ELLIPSE_SEGMENTS + 1;
    }
    return 0;
}

static void builtin_object_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    int i = 0;

    if (!object || !object->impl || !out_points) {
        return;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        const LineData* line = (const LineData*)object->impl;
        out_points[0] = line->p1;
        out_points[1] = line->p2;
        return;
    }

    if (object->type == GRAPHIC_OBJECT_RECT) {
        const RectData* rect = (const RectData*)object->impl;
        out_points[0] = vec2_make(rect->rect.x, rect->rect.y);
        out_points[1] = vec2_make(rect->rect.x + rect->rect.w, rect->rect.y);
        out_points[2] = vec2_make(rect->rect.x + rect->rect.w, rect->rect.y + rect->rect.h);
        out_points[3] = vec2_make(rect->rect.x, rect->rect.y + rect->rect.h);
        out_points[4] = out_points[0];
        return;
    }

    if (object->type == GRAPHIC_OBJECT_ELLIPSE) {
        const EllipseData* ellipse = (const EllipseData*)object->impl;
        float rx = ellipse->bounds.w * 0.5f;
        float ry = ellipse->bounds.h * 0.5f;
        Vec2 center = vec2_make(ellipse->bounds.x + rx, ellipse->bounds.y + ry);

        for (i = 0; i < ELLIPSE_SEGMENTS; ++i) {
            float t = (float)i / (float)ELLIPSE_SEGMENTS;
            float angle = t * 6.28318530718f;
            out_points[i] = vec2_make(center.x + cosf(angle) * rx,
                                      center.y + sinf(angle) * ry);
        }
        out_points[ELLIPSE_SEGMENTS] = out_points[0];
    }
}

static int builtin_object_serialize(const GraphicObject* object, GraphicPropertyBag* out_properties)
{
    return default_serialize_from_schema(object, out_properties);
}

static GraphicObject* make_line_from_bag(const GraphicPropertyBag* properties, GraphicStyle style)
{
    Vec2 p1 = {0.0f, 0.0f};
    Vec2 p2 = {0.0f, 0.0f};
    LineData* data = NULL;

    if (!graphic_property_bag_get(properties, "x1", &p1.x) ||
        !graphic_property_bag_get(properties, "y1", &p1.y) ||
        !graphic_property_bag_get(properties, "x2", &p2.x) ||
        !graphic_property_bag_get(properties, "y2", &p2.y)) {
        return NULL;
    }

    data = (LineData*)calloc(1, sizeof(*data));
    if (!data) {
        return NULL;
    }
    data->p1 = p1;
    data->p2 = p2;
    return object_alloc(GRAPHIC_OBJECT_LINE,
                        object_registry_lookup_by_type(GRAPHIC_OBJECT_LINE),
                        data,
                        style);
}

static GraphicObject* make_rect_from_bag(const GraphicPropertyBag* properties, GraphicStyle style)
{
    RectData* data = NULL;
    RectF rect = {0.0f, 0.0f, 0.0f, 0.0f};

    if (!graphic_property_bag_get(properties, "x", &rect.x) ||
        !graphic_property_bag_get(properties, "y", &rect.y) ||
        !graphic_property_bag_get(properties, "width", &rect.w) ||
        !graphic_property_bag_get(properties, "height", &rect.h) ||
        rect.w <= 0.0f || rect.h <= 0.0f) {
        return NULL;
    }

    data = (RectData*)calloc(1, sizeof(*data));
    if (!data) {
        return NULL;
    }
    data->rect = rect;
    return object_alloc(GRAPHIC_OBJECT_RECT,
                        object_registry_lookup_by_type(GRAPHIC_OBJECT_RECT),
                        data,
                        style);
}

static GraphicObject* make_ellipse_from_bag(const GraphicPropertyBag* properties, GraphicStyle style)
{
    EllipseData* data = NULL;
    RectF bounds = {0.0f, 0.0f, 0.0f, 0.0f};

    if (!graphic_property_bag_get(properties, "x", &bounds.x) ||
        !graphic_property_bag_get(properties, "y", &bounds.y) ||
        !graphic_property_bag_get(properties, "width", &bounds.w) ||
        !graphic_property_bag_get(properties, "height", &bounds.h) ||
        bounds.w <= 0.0f || bounds.h <= 0.0f) {
        return NULL;
    }

    data = (EllipseData*)calloc(1, sizeof(*data));
    if (!data) {
        return NULL;
    }
    data->bounds = bounds;
    return object_alloc(GRAPHIC_OBJECT_ELLIPSE,
                        object_registry_lookup_by_type(GRAPHIC_OBJECT_ELLIPSE),
                        data,
                        style);
}

static GraphicObject* builtin_object_deserialize_line(const GraphicPropertyBag* properties, GraphicStyle style)
{
    return make_line_from_bag(properties, style);
}

static GraphicObject* builtin_object_deserialize_rect(const GraphicPropertyBag* properties, GraphicStyle style)
{
    return make_rect_from_bag(properties, style);
}

static GraphicObject* builtin_object_deserialize_ellipse(const GraphicPropertyBag* properties, GraphicStyle style)
{
    return make_ellipse_from_bag(properties, style);
}

static GraphicObject* builtin_object_create_line(const void* init_data, GraphicStyle style)
{
    const Vec2* points = (const Vec2*)init_data;
    GraphicPropertyBag bag;

    graphic_property_bag_init(&bag);
    if (points) {
        graphic_property_bag_set(&bag, "x1", points[0].x);
        graphic_property_bag_set(&bag, "y1", points[0].y);
        graphic_property_bag_set(&bag, "x2", points[1].x);
        graphic_property_bag_set(&bag, "y2", points[1].y);
    }
    return builtin_object_deserialize_line(&bag, style);
}

static GraphicObject* builtin_object_create_rect(const void* init_data, GraphicStyle style)
{
    const RectF* rect = (const RectF*)init_data;
    GraphicPropertyBag bag;

    graphic_property_bag_init(&bag);
    if (rect) {
        graphic_property_bag_set(&bag, "x", rect->x);
        graphic_property_bag_set(&bag, "y", rect->y);
        graphic_property_bag_set(&bag, "width", rect->w);
        graphic_property_bag_set(&bag, "height", rect->h);
    }
    return builtin_object_deserialize_rect(&bag, style);
}

static GraphicObject* builtin_object_create_ellipse(const void* init_data, GraphicStyle style)
{
    const RectF* rect = (const RectF*)init_data;
    GraphicPropertyBag bag;

    graphic_property_bag_init(&bag);
    if (rect) {
        graphic_property_bag_set(&bag, "x", rect->x);
        graphic_property_bag_set(&bag, "y", rect->y);
        graphic_property_bag_set(&bag, "width", rect->w);
        graphic_property_bag_set(&bag, "height", rect->h);
    }
    return builtin_object_deserialize_ellipse(&bag, style);
}

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

static const GraphicObjectDescriptor g_builtin_descriptors[] = {
    {GRAPHIC_OBJECT_LINE,
     "line",
     "Line",
     builtin_object_create_line,
     default_clone_object,
     builtin_object_destroy,
     builtin_object_translate,
     builtin_object_get_bounds,
     builtin_object_hit_test,
     builtin_object_get_path_point_count,
     builtin_object_write_path_points,
     builtin_object_get_scalar,
     builtin_object_set_scalar,
     builtin_object_serialize,
     builtin_object_deserialize_line,
     g_line_schema,
     (int)(sizeof(g_line_schema) / sizeof(g_line_schema[0]))},
    {GRAPHIC_OBJECT_RECT,
     "rect",
     "Rectangle",
     builtin_object_create_rect,
     default_clone_object,
     builtin_object_destroy,
     builtin_object_translate,
     builtin_object_get_bounds,
     builtin_object_hit_test,
     builtin_object_get_path_point_count,
     builtin_object_write_path_points,
     builtin_object_get_scalar,
     builtin_object_set_scalar,
     builtin_object_serialize,
     builtin_object_deserialize_rect,
     g_rect_schema,
     (int)(sizeof(g_rect_schema) / sizeof(g_rect_schema[0]))},
    {GRAPHIC_OBJECT_ELLIPSE,
     "ellipse",
     "Ellipse",
     builtin_object_create_ellipse,
     default_clone_object,
     builtin_object_destroy,
     builtin_object_translate,
     builtin_object_get_bounds,
     builtin_object_hit_test,
     builtin_object_get_path_point_count,
     builtin_object_write_path_points,
     builtin_object_get_scalar,
     builtin_object_set_scalar,
     builtin_object_serialize,
     builtin_object_deserialize_ellipse,
     g_ellipse_schema,
     (int)(sizeof(g_ellipse_schema) / sizeof(g_ellipse_schema[0]))}
};

static void ensure_registry_builtins(void)
{
    size_t i = 0;

    object_registry_init();

    for (i = 0; i < sizeof(g_builtin_descriptors) / sizeof(g_builtin_descriptors[0]); ++i) {
        if (!object_registry_find_by_type_id(g_builtin_descriptors[i].type_id)) {
            register_object_type(&g_builtin_descriptors[i]);
        }
    }

    object_register_builtin_extensions();
}

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

const char* object_type_name(GraphicObjectType type)
{
    const GraphicObjectDescriptor* descriptor = NULL;

    ensure_registry_builtins();
    descriptor = object_registry_lookup_by_type(type);
    return descriptor ? descriptor->name : "Unknown";
}

const char* object_type_id(const GraphicObject* object)
{
    return (object && object->descriptor) ? object->descriptor->type_id : NULL;
}

int object_type_is(const GraphicObject* object, const char* type_id)
{
    return object && type_id && object->descriptor && object->descriptor->type_id &&
           strcmp(object->descriptor->type_id, type_id) == 0;
}

const GraphicPropertyDef* object_property_schema(const GraphicObject* object, int* out_count)
{
    if (out_count) {
        *out_count = 0;
    }
    if (!object || !object->descriptor) {
        return NULL;
    }
    if (out_count) {
        *out_count = object->descriptor->property_count;
    }
    return object->descriptor->property_schema;
}

GraphicObject* object_create(const char* type_id, const void* init_data, GraphicStyle style)
{
    const GraphicObjectDescriptor* descriptor = NULL;

    ensure_registry_builtins();
    descriptor = object_registry_lookup(type_id);
    if (!descriptor || !descriptor->create) {
        return NULL;
    }

    return descriptor->create(init_data, style);
}

GraphicObject* object_create_line(Vec2 p1, Vec2 p2, GraphicStyle style)
{
    Vec2 points[2] = {p1, p2};
    return object_create("line", points, style);
}

GraphicObject* object_create_rect(RectF rect, GraphicStyle style)
{
    return object_create("rect", &rect, style);
}

GraphicObject* object_create_ellipse(RectF bounds, GraphicStyle style)
{
    return object_create("ellipse", &bounds, style);
}

GraphicObject* object_clone(const GraphicObject* object)
{
    GraphicObject* clone = NULL;

    if (!object || !object->descriptor || !object->descriptor->clone) {
        return NULL;
    }

    clone = object->descriptor->clone(object);
    if (!clone) {
        return NULL;
    }

    clone->id = object->id;
    clone->layer_id = object->layer_id;
    clone->revision = object->revision;
    return clone;
}

void object_destroy(GraphicObject* object)
{
    if (object && object->descriptor && object->descriptor->destroy) {
        object->descriptor->destroy(object);
    }
}

void object_translate(GraphicObject* object, Vec2 delta)
{
    if (!object || !object->descriptor || !object->descriptor->translate) {
        return;
    }
    object->descriptor->translate(object, delta);
    object_bump_revision(object);
}

RectF object_get_bounds(const GraphicObject* object)
{
    RectF empty = {0.0f, 0.0f, 0.0f, 0.0f};

    if (!object || !object->descriptor || !object->descriptor->get_bounds) {
        return empty;
    }

    return object->descriptor->get_bounds(object);
}

int object_hit_test(const GraphicObject* object, Vec2 point, float tolerance)
{
    if (!object || !object->descriptor || !object->descriptor->hit_test) {
        return 0;
    }
    return object->descriptor->hit_test(object, point, tolerance);
}

int object_get_path_point_count(const GraphicObject* object)
{
    if (!object || !object->descriptor || !object->descriptor->get_path_point_count) {
        return 0;
    }
    return object->descriptor->get_path_point_count(object);
}

void object_write_path_points(const GraphicObject* object, Vec2* out_points)
{
    if (!object || !object->descriptor || !object->descriptor->write_path_points) {
        return;
    }
    object->descriptor->write_path_points(object, out_points);
}

int object_get_scalar(const GraphicObject* object, const char* key, float* out_value)
{
    if (!object || !object->descriptor || !object->descriptor->get_scalar) {
        return 0;
    }
    return object->descriptor->get_scalar(object, key, out_value);
}

int object_set_scalar(GraphicObject* object, const char* key, float value)
{
    if (!object || !object->descriptor || !object->descriptor->set_scalar) {
        return 0;
    }
    if (!object->descriptor->set_scalar(object, key, value)) {
        return 0;
    }
    object_bump_revision(object);
    return 1;
}

void object_set_stroke_color(GraphicObject* object, Color color)
{
    if (!object) {
        return;
    }
    object->style.stroke_color = color;
    object_bump_revision(object);
}

void object_set_stroke_width(GraphicObject* object, float stroke_width)
{
    if (!object) {
        return;
    }
    object->style.stroke_width = stroke_width;
    object_bump_revision(object);
}
