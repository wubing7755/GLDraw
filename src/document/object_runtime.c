#include <document/object.h>

#include <base/math2d.h>

#include "object_internal.h"

#include <stdlib.h>
#include <string.h>

static void object_bump_revision(GraphicObject* object)
{
    if (object) {
        object->revision++;
    }
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

void default_destroy_object(GraphicObject* object)
{
    if (!object) {
        return;
    }

    free(object->impl);
    free(object);
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
