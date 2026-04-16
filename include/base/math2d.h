/**
 * @file math2d.h
 * @brief Inline 2D math helpers shared by canvas/tools/render code.
 *
 * Role in project:
 * - Centralizes tiny vector/rectangle operations.
 * - Avoids repeated ad-hoc math across modules.
 *
 * Module relationships:
 * - Consumed by canvas transforms, tool delta handling, and rendering math.
 */
#ifndef GLDRAW_BASE_MATH2D_H
#define GLDRAW_BASE_MATH2D_H

#include <math.h>
#include <base/types.h>

/** Build a vector from two scalars. Complexity: `O(1)`. */
static inline Vec2 vec2_make(float x, float y)
{
    Vec2 value;
    value.x = x;
    value.y = y;
    return value;
}

/** Add two vectors component-wise. Complexity: `O(1)`. */
static inline Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return vec2_make(a.x + b.x, a.y + b.y);
}

/** Subtract two vectors component-wise. Complexity: `O(1)`. */
static inline Vec2 vec2_sub(Vec2 a, Vec2 b)
{
    return vec2_make(a.x - b.x, a.y - b.y);
}

/** Multiply a vector by a scalar. Complexity: `O(1)`. */
static inline Vec2 vec2_scale(Vec2 value, float scalar)
{
    return vec2_make(value.x * scalar, value.y * scalar);
}

/** Dot product of two vectors. Complexity: `O(1)`. */
static inline float vec2_dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

/** Squared length of a vector (avoids `sqrt`). Complexity: `O(1)`. */
static inline float vec2_length_sq(Vec2 value)
{
    return vec2_dot(value, value);
}

/** Euclidean vector length. Complexity: `O(1)`. */
static inline float vec2_length(Vec2 value)
{
    return sqrtf(vec2_length_sq(value));
}

/**
 * Clamp a scalar into `[min_value, max_value]`.
 * Edge case: behavior assumes `min_value <= max_value`.
 * Complexity: `O(1)`.
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

/** Rectangle left edge (`x`). Complexity: `O(1)`. */
static inline float rectf_left(const RectF* rect)
{
    return rect->x;
}

/** Rectangle right edge (`x + w`). Complexity: `O(1)`. */
static inline float rectf_right(const RectF* rect)
{
    return rect->x + rect->w;
}

/** Rectangle bottom edge (`y`). Complexity: `O(1)`. */
static inline float rectf_bottom(const RectF* rect)
{
    return rect->y;
}

/** Rectangle top edge (`y + h`). Complexity: `O(1)`. */
static inline float rectf_top(const RectF* rect)
{
    return rect->y + rect->h;
}

/**
 * Inclusive point-in-rectangle test.
 * Risk: caller must pass a non-null `rect`.
 * Complexity: `O(1)`.
 */
static inline int rectf_contains_point(const RectF* rect, Vec2 point)
{
    return point.x >= rect->x &&
           point.x <= rect->x + rect->w &&
           point.y >= rect->y &&
           point.y <= rect->y + rect->h;
}

#endif /* GLDRAW_BASE_MATH2D_H */
