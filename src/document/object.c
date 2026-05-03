#include <document/object.h>

#include <base/math2d.h>

#include "object_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJECT_REGISTRY_INITIAL_CAPACITY 8

typedef struct {
    GraphicObjectDescriptor *descriptors;
    int count;
    int capacity;
    unsigned int next_dynamic_type;
    int initialized;
} ObjectRegistryState;

static ObjectRegistryState g_registry = {0};

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

int style_get_scalar(const GraphicObject* object, const char* key, float* out_value)
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

int style_set_scalar(GraphicObject* object, const char* key, float value)
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

GraphicObject* object_alloc(GraphicObjectType type,
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
    if (g_registry.count >= g_registry.capacity) {
        int new_capacity = g_registry.capacity > 0 ? g_registry.capacity * 2
                                                    : OBJECT_REGISTRY_INITIAL_CAPACITY;
        GraphicObjectDescriptor *new_descriptors =
            (GraphicObjectDescriptor *)realloc(g_registry.descriptors,
                                               (size_t)new_capacity *
                                                   sizeof(g_registry.descriptors[0]));
        if (!new_descriptors) {
            return NULL;
        }
        g_registry.descriptors = new_descriptors;
        g_registry.capacity = new_capacity;
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
    g_registry.next_dynamic_type = 100u;
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
    return object_registry_find_by_type_id(type_id);
}

const GraphicObjectDescriptor* object_registry_lookup_by_type(GraphicObjectType type)
{
    return object_registry_find_by_type(type);
}

int object_registry_count(void)
{
    return g_registry.count;
}

const GraphicObjectDescriptor* object_registry_at(int index)
{
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

int default_serialize_from_schema(const GraphicObject* object, GraphicPropertyBag* out_properties)
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

GraphicObject* default_clone_object(const GraphicObject* object)
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

    descriptor = object_registry_lookup(type_id);
    if (!descriptor || !descriptor->create) {
        return NULL;
    }

    return descriptor->create(init_data, style);
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
