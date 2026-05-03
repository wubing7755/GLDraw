/**
 * @file object.h
 * @brief Graphic object descriptor, registry, and runtime object APIs.
 */
#ifndef GLDRAW_DOCUMENT_OBJECT_H
#define GLDRAW_DOCUMENT_OBJECT_H

#include <base/types.h>

#include <stddef.h>

typedef unsigned int GraphicObjectType;
typedef unsigned int LayerId;

#define GRAPHIC_OBJECT_INVALID 0u
#define GRAPHIC_OBJECT_LINE 1u
#define GRAPHIC_OBJECT_RECT 2u
#define GRAPHIC_OBJECT_ELLIPSE 3u

#define GRAPHIC_OBJECT_MAX_PROPERTIES 32
#define GRAPHIC_OBJECT_MAX_TYPES 32

typedef struct {
    Color stroke_color;
    float stroke_width;
} GraphicStyle;

typedef enum {
    GRAPHIC_PROPERTY_FLOAT = 0
} GraphicPropertyType;

typedef struct {
    const char* name;
    GraphicPropertyType type;
    float min_value;
    float max_value;
    float step;
    float inc_per_pixel;
} GraphicPropertyDef;

typedef struct {
    char name[32];
    float value;
} GraphicPropertyValue;

typedef struct {
    GraphicPropertyValue values[GRAPHIC_OBJECT_MAX_PROPERTIES];
    int count;
} GraphicPropertyBag;

typedef struct GraphicObject GraphicObject;
typedef struct GraphicObjectDescriptor GraphicObjectDescriptor;

typedef GraphicObject* (*GraphicObjectCreateFn)(const void* init_data, GraphicStyle style);
typedef GraphicObject* (*GraphicObjectCloneFn)(const GraphicObject* object);
typedef void (*GraphicObjectDestroyFn)(GraphicObject* object);
typedef void (*GraphicObjectTranslateFn)(GraphicObject* object, Vec2 delta);
typedef RectF (*GraphicObjectGetBoundsFn)(const GraphicObject* object);
typedef int (*GraphicObjectHitTestFn)(const GraphicObject* object, Vec2 point, float tolerance);
typedef int (*GraphicObjectGetPathPointCountFn)(const GraphicObject* object);
typedef void (*GraphicObjectWritePathPointsFn)(const GraphicObject* object, Vec2* out_points);
typedef int (*GraphicObjectGetScalarFn)(const GraphicObject* object, const char* key, float* out_value);
typedef int (*GraphicObjectSetScalarFn)(GraphicObject* object, const char* key, float value);
typedef int (*GraphicObjectSerializeFn)(const GraphicObject* object, GraphicPropertyBag* out_properties);
typedef GraphicObject* (*GraphicObjectDeserializeFn)(const GraphicPropertyBag* properties,
                                                     GraphicStyle style);

struct GraphicObjectDescriptor {
    GraphicObjectType type;
    const char* type_id;
    const char* name;
    GraphicObjectCreateFn create;
    GraphicObjectCloneFn clone;
    GraphicObjectDestroyFn destroy;
    GraphicObjectTranslateFn translate;
    GraphicObjectGetBoundsFn get_bounds;
    GraphicObjectHitTestFn hit_test;
    GraphicObjectGetPathPointCountFn get_path_point_count;
    GraphicObjectWritePathPointsFn write_path_points;
    GraphicObjectGetScalarFn get_scalar;
    GraphicObjectSetScalarFn set_scalar;
    GraphicObjectSerializeFn serialize;
    GraphicObjectDeserializeFn deserialize;
    const GraphicPropertyDef* property_schema;
    int property_count;
};

struct GraphicObject {
    ObjectId id;
    LayerId layer_id;
    GraphicObjectType type;
    const GraphicObjectDescriptor* descriptor;
    void* impl;
    GraphicStyle style;
    unsigned int revision;
};

GraphicStyle object_default_style(void);

void object_registry_init(void);
int register_object_type(const GraphicObjectDescriptor* descriptor);
const GraphicObjectDescriptor* object_registry_lookup(const char* type_id);
const GraphicObjectDescriptor* object_registry_lookup_by_type(GraphicObjectType type);
int object_registry_count(void);
const GraphicObjectDescriptor* object_registry_at(int index);

void graphic_property_bag_init(GraphicPropertyBag* bag);
int graphic_property_bag_set(GraphicPropertyBag* bag, const char* name, float value);
int graphic_property_bag_get(const GraphicPropertyBag* bag, const char* name, float* out_value);

const char* object_type_name(GraphicObjectType type);
const char* object_type_id(const GraphicObject* object);
int object_type_is(const GraphicObject* object, const char* type_id);
const GraphicPropertyDef* object_property_schema(const GraphicObject* object, int* out_count);
GraphicObject* object_create(const char* type_id, const void* init_data, GraphicStyle style);

GraphicObject* object_create_line(Vec2 p1, Vec2 p2, GraphicStyle style);
GraphicObject* object_create_rect(RectF rect, GraphicStyle style);
GraphicObject* object_create_ellipse(RectF bounds, GraphicStyle style);

GraphicObject* object_clone(const GraphicObject* object);
void object_destroy(GraphicObject* object);
void object_translate(GraphicObject* object, Vec2 delta);
RectF object_get_bounds(const GraphicObject* object);
int object_hit_test(const GraphicObject* object, Vec2 point, float tolerance);
int object_get_path_point_count(const GraphicObject* object);
void object_write_path_points(const GraphicObject* object, Vec2* out_points);
int object_get_scalar(const GraphicObject* object, const char* key, float* out_value);
int object_set_scalar(GraphicObject* object, const char* key, float value);
void object_set_stroke_color(GraphicObject* object, Color color);
void object_set_stroke_width(GraphicObject* object, float stroke_width);

#endif /* GLDRAW_DOCUMENT_OBJECT_H */
