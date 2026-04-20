/**
 * @file object.h
 * @brief 图元对象多态接口与类型定义。
 */
#ifndef GLDRAW_DOCUMENT_OBJECT_H
#define GLDRAW_DOCUMENT_OBJECT_H

#include <base/types.h>

/**
 * @enum GraphicObjectType
 * @brief 内置图元类型枚举。
 */
typedef enum {
    GRAPHIC_OBJECT_LINE = 0,
    GRAPHIC_OBJECT_RECT,
    GRAPHIC_OBJECT_ELLIPSE
} GraphicObjectType;

/**
 * @struct GraphicStyle
 * @brief 图元通用描边样式。
 *
 * @member stroke_color 描边颜色。
 * @member stroke_width 描边宽度（世界坐标单位）。
 */
typedef struct {
    Color stroke_color;
    float stroke_width;
} GraphicStyle;

typedef struct GraphicObject GraphicObject;
typedef struct GraphicObjectVTable GraphicObjectVTable;

/**
 * @struct GraphicObjectVTable
 * @brief 图元多态行为表。
 *
 * @member name 类型名称。
 * @member destroy 析构实现。
 * @member translate 平移实现。
 * @member get_bounds 包围盒计算实现。
 * @member hit_test 命中测试实现。
 * @member get_path_point_count 路径采样点数量实现。
 * @member write_path_points 路径采样点写出实现。
 * @member get_scalar 标量属性读取实现。
 * @member set_scalar 标量属性写入实现。
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
 * @brief 运行时图元对象。
 *
 * @member id 文档内对象 ID。
 * @member type 图元类型。
 * @member vtable 多态行为表。
 * @member impl 类型私有实现数据。
 * @member style 绘制样式。
 * @member revision 对象修订号。
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
 * @brief 获取新建对象默认样式。
 * @return 默认 `GraphicStyle`。
 */
GraphicStyle object_default_style(void);

/**
 * @brief 获取图元类型名称。
 * @param type 图元类型枚举值。
 * @return 对应名称字符串。
 */
const char* object_type_name(GraphicObjectType type);

/**
 * @brief 创建线段对象。
 * @param p1 起点。
 * @param p2 终点。
 * @param style 描边样式。
 * @return 成功返回对象指针，失败返回 `NULL`。
 */
GraphicObject* object_create_line(Vec2 p1, Vec2 p2, GraphicStyle style);

/**
 * @brief 创建矩形对象。
 * @param rect 矩形边界。
 * @param style 描边样式。
 * @return 成功返回对象指针，失败返回 `NULL`。
 */
GraphicObject* object_create_rect(RectF rect, GraphicStyle style);

/**
 * @brief 创建椭圆对象。
 * @param bounds 包围矩形。
 * @param style 描边样式。
 * @return 成功返回对象指针，失败返回 `NULL`。
 */
GraphicObject* object_create_ellipse(RectF bounds, GraphicStyle style);

/**
 * @brief 深拷贝图元对象。
 * @param object 源对象。
 * @return 成功返回新对象，失败返回 `NULL`。
 */
GraphicObject* object_clone(const GraphicObject* object);

/**
 * @brief 销毁图元对象。
 * @param object 目标对象（可为 `NULL`）。
 * @return 无。
 */
void object_destroy(GraphicObject* object);

/**
 * @brief 平移图元几何。
 * @param object 目标对象。
 * @param delta 平移增量。
 * @return 无。
 */
void object_translate(GraphicObject* object, Vec2 delta);

/**
 * @brief 获取图元轴对齐包围盒。
 * @param object 目标对象。
 * @return 包围盒；参数非法时返回空矩形。
 */
RectF object_get_bounds(const GraphicObject* object);

/**
 * @brief 图元命中测试。
 * @param object 目标对象。
 * @param point 世界坐标测试点。
 * @param tolerance 命中容差。
 * @return 命中返回非零，否则返回 0。
 */
int object_hit_test(const GraphicObject* object, Vec2 point, float tolerance);

/**
 * @brief 获取图元路径采样点数量。
 * @param object 目标对象。
 * @return 路径点数量。
 */
int object_get_path_point_count(const GraphicObject* object);

/**
 * @brief 写出图元路径采样点。
 * @param object 目标对象。
 * @param out_points 输出数组。
 * @return 无。
 */
void object_write_path_points(const GraphicObject* object, Vec2* out_points);

/**
 * @brief 读取图元标量属性。
 * @param object 目标对象。
 * @param key 属性键名。
 * @param out_value 属性输出值。
 * @return 支持该键并读取成功返回非零，否则返回 0。
 */
int object_get_scalar(const GraphicObject* object, const char* key, float* out_value);

/**
 * @brief 写入图元标量属性。
 * @param object 目标对象。
 * @param key 属性键名。
 * @param value 新值。
 * @return 写入成功返回非零，否则返回 0。
 */
int object_set_scalar(GraphicObject* object, const char* key, float value);

/**
 * @brief 设置图元描边颜色。
 * @param object 目标对象。
 * @param color 新颜色。
 * @return 无。
 */
void object_set_stroke_color(GraphicObject* object, Color color);

/**
 * @brief 设置图元描边宽度。
 * @param object 目标对象。
 * @param stroke_width 新宽度。
 * @return 无。
 */
void object_set_stroke_width(GraphicObject* object, float stroke_width);

#endif /* GLDRAW_DOCUMENT_OBJECT_H */
