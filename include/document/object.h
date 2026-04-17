/**
 * @file object.h
 * @brief Polymorphic drawable object definitions and operations.
 *
 * Role in project:
 * - Defines object type IDs, style data, and vtable contracts.
 * - Provides creation, mutation, querying, and cloning APIs.
 *
 * Module relationships:
 * - Used by document storage, canvas picking, renderer path extraction, and persistence.
 */
#ifndef GLDRAW_DOCUMENT_OBJECT_H
#define GLDRAW_DOCUMENT_OBJECT_H

#include <base/types.h>

/** Supported built-in object kinds. */
typedef enum {
    GRAPHIC_OBJECT_LINE = 0,
    GRAPHIC_OBJECT_RECT,
    GRAPHIC_OBJECT_ELLIPSE
} GraphicObjectType;

/** Shared drawing style carried by each object instance. */
typedef struct {
    Color stroke_color;
    float stroke_width;
} GraphicStyle;

typedef struct GraphicObject GraphicObject;
typedef struct GraphicObjectVTable GraphicObjectVTable;

struct GraphicObjectVTable {
    const char* name;
    void (*destroy)(GraphicObject* object);
    void (*translate)(GraphicObject* object, Vec2 delta);
    RectF (*get_bounds)(const GraphicObject* object);
    int (*hit_test)(const GraphicObject* object, Vec2 point, float tolerance);
    int (*get_path_point_count)(const GraphicObject* object);
    void (*write_path_points)(const GraphicObject* object, Vec2* out_points);
    int (*get_scalar)(const GraphicObject* object, const char* key, float* out_value);
    int (*set_scalar)(GraphicObject* object, const char* key, float value);
};

struct GraphicObject {
    ObjectId id;
    GraphicObjectType type;
    const GraphicObjectVTable* vtable;
    void* impl;
    GraphicStyle style;
    unsigned int revision;
};

/** Return default style values for newly created objects. Complexity: `O(1)`. */
GraphicStyle object_default_style(void);
/** Return human-readable type name. Complexity: `O(1)`. */
const char* object_type_name(GraphicObjectType type);

/** Create a line object; returns `NULL` on allocation failure. Complexity: `O(1)`. */
GraphicObject* object_create_line(Vec2 p1, Vec2 p2, GraphicStyle style);
/** Create a rectangle object; returns `NULL` on allocation failure. Complexity: `O(1)`. */
GraphicObject* object_create_rect(RectF rect, GraphicStyle style);
/** Create an ellipse object; returns `NULL` on allocation failure. Complexity: `O(1)`. */
GraphicObject* object_create_ellipse(RectF bounds, GraphicStyle style);
/** Deep clone object payload/style/metadata; returns `NULL` on failure. Complexity: `O(1)`. */
GraphicObject* object_clone(const GraphicObject* object);

/** Destroy object via vtable; safe no-op for invalid input. Complexity: `O(1)`. */
void object_destroy(GraphicObject* object);
/** Translate geometry in world space and bump object revision. Complexity: `O(1)`. */
void object_translate(GraphicObject* object, Vec2 delta);
/** Get object axis-aligned bounds; returns empty rect for invalid input. Complexity: `O(1)`. */
RectF object_get_bounds(const GraphicObject* object);
/** Hit-test a world point against object geometry. Complexity: `O(1)`. */
int object_hit_test(const GraphicObject* object, Vec2 point, float tolerance);
/** Get polyline sample count for rendering. Complexity: `O(1)`. */
int object_get_path_point_count(const GraphicObject* object);
/** Write polyline points into caller-provided buffer. Complexity: `O(k)`. */
void object_write_path_points(const GraphicObject* object, Vec2* out_points);
/** Query scalar property by key. Returns 1 when key is supported. Complexity: `O(1)`. */
int object_get_scalar(const GraphicObject* object, const char* key, float* out_value);
/** Set scalar property by key and bump revision on success. Complexity: `O(1)`. */
int object_set_scalar(GraphicObject* object, const char* key, float value);
/** Update stroke color and bump revision. Complexity: `O(1)`. */
void object_set_stroke_color(GraphicObject* object, Color color);
/** Update stroke width and bump revision. Complexity: `O(1)`. */
void object_set_stroke_width(GraphicObject* object, float stroke_width);

#endif /* GLDRAW_DOCUMENT_OBJECT_H */
