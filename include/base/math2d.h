#ifndef GLDRAW_BASE_MATH2D_H
#define GLDRAW_BASE_MATH2D_H

#include <math.h>
#include <base/types.h>

static inline Vec2 vec2_make(float x, float y)
{
    Vec2 value;
    value.x = x;
    value.y = y;
    return value;
}

static inline Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return vec2_make(a.x + b.x, a.y + b.y);
}

static inline Vec2 vec2_sub(Vec2 a, Vec2 b)
{
    return vec2_make(a.x - b.x, a.y - b.y);
}

static inline Vec2 vec2_scale(Vec2 value, float scalar)
{
    return vec2_make(value.x * scalar, value.y * scalar);
}

static inline float vec2_dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

static inline float vec2_length_sq(Vec2 value)
{
    return vec2_dot(value, value);
}

static inline float vec2_length(Vec2 value)
{
    return sqrtf(vec2_length_sq(value));
}

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

static inline float rectf_left(const RectF* rect)
{
    return rect->x;
}

static inline float rectf_right(const RectF* rect)
{
    return rect->x + rect->w;
}

static inline float rectf_bottom(const RectF* rect)
{
    return rect->y;
}

static inline float rectf_top(const RectF* rect)
{
    return rect->y + rect->h;
}

static inline int rectf_contains_point(const RectF* rect, Vec2 point)
{
    return point.x >= rect->x &&
           point.x <= rect->x + rect->w &&
           point.y >= rect->y &&
           point.y <= rect->y + rect->h;
}

#endif /* GLDRAW_BASE_MATH2D_H */
