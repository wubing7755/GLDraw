/**
 * @file math2d.h
 * @brief 2D math inline utility functions.
 *
 * Provides common basics: vector operations, scalar clamping, and rectangle tests.
 * All functions are `static inline` to reduce cross-module duplicate code.
 */
#ifndef GLDRAW_BASE_MATH2D_H
#define GLDRAW_BASE_MATH2D_H

#include <math.h>
#include <base/types.h>

/**
 * @brief Construct a 2D vector.
 * @param x X component.
 * @param y Y component.
 * @return The constructed `Vec2`.
 */
static inline Vec2 vec2_make(float x, float y)
{
    Vec2 value;
    value.x = x;
    value.y = y;
    return value;
}

/**
 * @brief Component-wise addition of two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return `a + b`.
 */
static inline Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return vec2_make(a.x + b.x, a.y + b.y);
}

/**
 * @brief Component-wise subtraction of two vectors.
 * @param a Vector being subtracted.
 * @param b Vector to subtract.
 * @return `a - b`.
 */
static inline Vec2 vec2_sub(Vec2 a, Vec2 b)
{
    return vec2_make(a.x - b.x, a.y - b.y);
}

/**
 * @brief Scale a vector by a scalar.
 * @param value Input vector.
 * @param scalar Scale factor.
 * @return `value * scalar`.
 */
static inline Vec2 vec2_scale(Vec2 value, float scalar)
{
    return vec2_make(value.x * scalar, value.y * scalar);
}

/**
 * @brief Compute the dot product of two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return Dot product `a.x * b.x + a.y * b.y`.
 */
static inline float vec2_dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

/**
 * @brief Compute the squared length of a vector.
 * @param value Input vector.
 * @return Squared length (avoids `sqrt` overhead).
 */
static inline float vec2_length_sq(Vec2 value)
{
    return vec2_dot(value, value);
}

/**
 * @brief Compute the Euclidean length of a vector.
 * @param value Input vector.
 * @return Vector length.
 */
static inline float vec2_length(Vec2 value)
{
    return sqrtf(vec2_length_sq(value));
}

/**
 * @brief Clamp a scalar to the closed interval `[min_value, max_value]`.
 * @param value Input value.
 * @param min_value Lower bound.
 * @param max_value Upper bound.
 * @return Clamped value.
 * @note Callers must ensure `min_value <= max_value`.
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
 * @brief Get the left edge of a rectangle.
 * @param rect Input rectangle.
 * @return Left edge `x`.
 */
static inline float rectf_left(const RectF* rect)
{
    return rect->x;
}

/**
 * @brief Get the right edge of a rectangle.
 * @param rect Input rectangle.
 * @return Right edge `x + w`.
 */
static inline float rectf_right(const RectF* rect)
{
    return rect->x + rect->w;
}

/**
 * @brief Get the bottom edge of a rectangle.
 * @param rect Input rectangle.
 * @return Bottom edge `y`.
 */
static inline float rectf_bottom(const RectF* rect)
{
    return rect->y;
}

/**
 * @brief Get the top edge of a rectangle.
 * @param rect Input rectangle.
 * @return Top edge `y + h`.
 */
static inline float rectf_top(const RectF* rect)
{
    return rect->y + rect->h;
}

/**
 * @brief Check whether a point is inside the rectangle (including the boundary).
 * @param rect Input rectangle.
 * @param point Point to test.
 * @return Non-zero if the point is inside, zero otherwise.
 */
static inline int rectf_contains_point(const RectF* rect, Vec2 point)
{
    return point.x >= rect->x &&
           point.x <= rect->x + rect->w &&
           point.y >= rect->y &&
           point.y <= rect->y + rect->h;
}

#endif /* GLDRAW_BASE_MATH2D_H */
