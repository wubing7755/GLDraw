#include "object_internal.h"

#include <stdio.h>
#include <string.h>

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
