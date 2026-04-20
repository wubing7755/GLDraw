/**
 * @file math2d.h
 * @brief 2D 数学内联工具函数。
 *
 * 本文件提供向量运算、标量夹取和矩形判定等常用基础能力。
 * 所有函数均为 `static inline`，用于减少跨模块重复代码。
 */
#ifndef GLDRAW_BASE_MATH2D_H
#define GLDRAW_BASE_MATH2D_H

#include <math.h>
#include <base/types.h>

/**
 * @brief 构造二维向量。
 * @param x X 分量。
 * @param y Y 分量。
 * @return 构造后的 `Vec2`。
 */
static inline Vec2 vec2_make(float x, float y)
{
    Vec2 value;
    value.x = x;
    value.y = y;
    return value;
}

/**
 * @brief 逐分量相加两个向量。
 * @param a 第一个向量。
 * @param b 第二个向量。
 * @return `a + b`。
 */
static inline Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return vec2_make(a.x + b.x, a.y + b.y);
}

/**
 * @brief 逐分量相减两个向量。
 * @param a 被减向量。
 * @param b 减数向量。
 * @return `a - b`。
 */
static inline Vec2 vec2_sub(Vec2 a, Vec2 b)
{
    return vec2_make(a.x - b.x, a.y - b.y);
}

/**
 * @brief 向量按标量缩放。
 * @param value 输入向量。
 * @param scalar 缩放系数。
 * @return `value * scalar`。
 */
static inline Vec2 vec2_scale(Vec2 value, float scalar)
{
    return vec2_make(value.x * scalar, value.y * scalar);
}

/**
 * @brief 计算两个向量的点积。
 * @param a 第一个向量。
 * @param b 第二个向量。
 * @return 点积值 `a.x * b.x + a.y * b.y`。
 */
static inline float vec2_dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

/**
 * @brief 计算向量长度平方。
 * @param value 输入向量。
 * @return 长度平方值（避免 `sqrt` 开销）。
 */
static inline float vec2_length_sq(Vec2 value)
{
    return vec2_dot(value, value);
}

/**
 * @brief 计算向量欧氏长度。
 * @param value 输入向量。
 * @return 向量长度。
 */
static inline float vec2_length(Vec2 value)
{
    return sqrtf(vec2_length_sq(value));
}

/**
 * @brief 将标量约束到闭区间 `[min_value, max_value]`。
 * @param value 输入值。
 * @param min_value 下界。
 * @param max_value 上界。
 * @return 夹取后的值。
 * @note 调用方应保证 `min_value <= max_value`。
 */
static inline float clampf(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

/**
 * @brief 获取矩形左边界。
 * @param rect 输入矩形。
 * @return 左边界 `x`。
 */
static inline float rectf_left(const RectF* rect)
{
    return rect->x;
}

/**
 * @brief 获取矩形右边界。
 * @param rect 输入矩形。
 * @return 右边界 `x + w`。
 */
static inline float rectf_right(const RectF* rect)
{
    return rect->x + rect->w;
}

/**
 * @brief 获取矩形下边界。
 * @param rect 输入矩形。
 * @return 下边界 `y`。
 */
static inline float rectf_bottom(const RectF* rect)
{
    return rect->y;
}

/**
 * @brief 获取矩形上边界。
 * @param rect 输入矩形。
 * @return 上边界 `y + h`。
 */
static inline float rectf_top(const RectF* rect)
{
    return rect->y + rect->h;
}

/**
 * @brief 判断点是否位于矩形内部（含边界）。
 * @param rect 输入矩形。
 * @param point 待检测点。
 * @return 点在矩形内返回非零，否则返回 0。
 */
static inline int rectf_contains_point(const RectF* rect, Vec2 point)
{
    return point.x >= rect->x &&
           point.x <= rect->x + rect->w &&
           point.y >= rect->y &&
           point.y <= rect->y + rect->h;
}

#endif /* GLDRAW_BASE_MATH2D_H */
