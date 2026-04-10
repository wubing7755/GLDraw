#ifndef GLDRAW_DOCUMENT_OBJECT_H
#define GLDRAW_DOCUMENT_OBJECT_H

#include <base/types.h>

typedef enum {
    GRAPHIC_OBJECT_LINE = 0,
    GRAPHIC_OBJECT_RECT,
    GRAPHIC_OBJECT_ELLIPSE
} GraphicObjectType;

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

GraphicStyle object_default_style(void);
const char* object_type_name(GraphicObjectType type);

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
