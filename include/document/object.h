/**
 * @file object.h
 * @brief Graphic object polymorphic interface and type definitions.
 */
#ifndef GLDRAW_DOCUMENT_OBJECT_H
#define GLDRAW_DOCUMENT_OBJECT_H

#include <base/types.h>

/**
 * @enum GraphicObjectType
 * @brief Built-in graphic object type enum.
 */
typedef enum {
    GRAPHIC_OBJECT_LINE = 0,
    GRAPHIC_OBJECT_RECT,
    GRAPHIC_OBJECT_ELLIPSE
} GraphicObjectType;

/**
 * @struct GraphicStyle
 * @brief Common stroke style for graphic objects.
 *
 * @member stroke_color Stroke color.
 * @member stroke_width Stroke width (in world coordinate units).
 */
typedef struct {
    Color stroke_color;
    float stroke_width;
} GraphicStyle;

typedef struct GraphicObject GraphicObject;
typedef struct GraphicObjectVTable GraphicObjectVTable;

/**
 * @struct GraphicObjectVTable
 * @brief Graphic object polymorphic behavior table.
 *
 * @member name Type name.
 * @member destroy Destructor implementation.
 * @member translate Translation implementation.
 * @member get_bounds Bounding box computation implementation.
 * @member hit_test Hit-test implementation.
 * @member get_path_point_count Path sample point count implementation.
 * @member write_path_points Path sample point write-out implementation.
 * @member get_scalar Scalar property read implementation.
 * @member set_scalar Scalar property write implementation.
 */
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

/**
 * @struct GraphicObject
 * @brief Runtime graphic object.
 *
 * @member id Object ID within the document.
 * @member type Object type.
 * @member vtable Polymorphic behavior table.
 * @member impl Type-private implementation data.
 * @member style Drawing style.
 * @member revision Object revision number.
 */
struct GraphicObject {
    ObjectId id;
    GraphicObjectType type;
    const GraphicObjectVTable* vtable;
    void* impl;
    GraphicStyle style;
    unsigned int revision;
};

/**
 * @brief Get the default style for newly created objects.
 * @return Default `GraphicStyle`.
 */
GraphicStyle object_default_style(void);

/**
 * @brief Get the type name of a graphic object.
 * @param type Graphic object type enum value.
 * @return Corresponding name string.
 */
const char* object_type_name(GraphicObjectType type);

/**
 * @brief Create a line object.
 * @param p1 Start point.
 * @param p2 End point.
 * @param style Stroke style.
 * @return Object pointer on success, `NULL` on failure.
 */
GraphicObject* object_create_line(Vec2 p1, Vec2 p2, GraphicStyle style);

/**
 * @brief Create a rectangle object.
 * @param rect Rectangle boundary.
 * @param style Stroke style.
 * @return Object pointer on success, `NULL` on failure.
 */
GraphicObject* object_create_rect(RectF rect, GraphicStyle style);

/**
 * @brief Create an ellipse object.
 * @param bounds Bounding rectangle.
 * @param style Stroke style.
 * @return Object pointer on success, `NULL` on failure.
 */
GraphicObject* object_create_ellipse(RectF bounds, GraphicStyle style);

/**
 * @brief Deep-copy a graphic object.
 * @param object Source object.
 * @return New object on success, `NULL` on failure.
 */
GraphicObject* object_clone(const GraphicObject* object);

/**
 * @brief Destroy a graphic object.
 * @param object Target object (may be `NULL`).
 * @return No return value.
 */
void object_destroy(GraphicObject* object);

/**
 * @brief Translate the object geometry.
 * @param object Target object.
 * @param delta Translation delta.
 * @return No return value.
 */
void object_translate(GraphicObject* object, Vec2 delta);

/**
 * @brief Get the object's axis-aligned bounding box.
 * @param object Target object.
 * @return Bounding box; returns an empty rectangle if parameters are invalid.
 */
RectF object_get_bounds(const GraphicObject* object);

/**
 * @brief Object hit-test.
 * @param object Target object.
 * @param point World-coordinate test point.
 * @param tolerance Hit tolerance.
 * @return Non-zero on hit, zero otherwise.
 */
int object_hit_test(const GraphicObject* object, Vec2 point, float tolerance);

/**
 * @brief Get the number of path sample points for the object.
 * @param object Target object.
 * @return Path point count.
 */
int object_get_path_point_count(const GraphicObject* object);

/**
 * @brief Write out the object's path sample points.
 * @param object Target object.
 * @param out_points Output array.
 * @return No return value.
 */
void object_write_path_points(const GraphicObject* object, Vec2* out_points);

/**
 * @brief Read an object scalar property.
 * @param object Target object.
 * @param key Property key name.
 * @param out_value Property output value.
 * @return Non-zero if the key is supported and read succeeded, zero otherwise.
 */
int object_get_scalar(const GraphicObject* object, const char* key, float* out_value);

/**
 * @brief Write an object scalar property.
 * @param object Target object.
 * @param key Property key name.
 * @param value New value.
 * @return Non-zero on success, zero otherwise.
 */
int object_set_scalar(GraphicObject* object, const char* key, float value);

/**
 * @brief Set the object stroke color.
 * @param object Target object.
 * @param color New color.
 * @return No return value.
 */
void object_set_stroke_color(GraphicObject* object, Color color);

/**
 * @brief Set the object stroke width.
 * @param object Target object.
 * @param stroke_width New width.
 * @return No return value.
 */
void object_set_stroke_width(GraphicObject* object, float stroke_width);

#endif /* GLDRAW_DOCUMENT_OBJECT_H */
