/**
 * @file types.h
 * @brief Small shared value types used by all core modules.
 *
 * Role in project:
 * - Provides stable primitive structs for geometry and color.
 * - Reduces cross-module coupling on third-party math/color APIs.
 *
 * Module relationships:
 * - Included by document, canvas, tools, render, and UI headers.
 */
#ifndef GLDRAW_BASE_TYPES_H
#define GLDRAW_BASE_TYPES_H

/** Stable object identity in a document. */
typedef unsigned int ObjectId;

/** 2D vector in world/screen space. */
typedef struct {
    float x;
    float y;
} Vec2;

/** RGBA color in normalized float range `[0, 1]`. */
typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

/** Floating-point rectangle (`x`, `y`, `width`, `height`). */
typedef struct {
    float x;
    float y;
    float w;
    float h;
} RectF;

#endif /* GLDRAW_BASE_TYPES_H */
