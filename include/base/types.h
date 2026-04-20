/**
 * @file types.h
 * @brief 项目共享的基础值类型定义。
 *
 * 该文件只声明轻量级 POD 类型，不包含任何行为逻辑，
 * 供 document/canvas/tools/render/ui 等模块复用。
 */
#ifndef GLDRAW_BASE_TYPES_H
#define GLDRAW_BASE_TYPES_H

/**
 * @typedef ObjectId
 * @brief 文档内对象的稳定标识符类型。
 */
typedef unsigned int ObjectId;

/**
 * @struct Vec2
 * @brief 二维向量（可用于世界坐标或屏幕坐标）。
 *
 * @member x X 分量。
 * @member y Y 分量。
 */
typedef struct {
    float x;
    float y;
} Vec2;

/**
 * @struct Color
 * @brief RGBA 颜色（分量范围通常为 `[0, 1]`）。
 *
 * @member r 红色分量。
 * @member g 绿色分量。
 * @member b 蓝色分量。
 * @member a 透明度分量。
 */
typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

/**
 * @struct RectF
 * @brief 浮点矩形（x/y/w/h）。
 *
 * @member x 左下角（或左上角，取决于调用方坐标系）X 坐标。
 * @member y 左下角（或左上角，取决于调用方坐标系）Y 坐标。
 * @member w 宽度。
 * @member h 高度。
 */
typedef struct {
    float x;
    float y;
    float w;
    float h;
} RectF;

#endif /* GLDRAW_BASE_TYPES_H */
