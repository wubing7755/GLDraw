/**
 * @file types.h
 * @brief Project-shared basic value type definitions.
 *
 * This file declares lightweight POD types only, with no behavior logic.
 * Shared by document/canvas/tools/render/ui and other modules.
 */
#ifndef GLDRAW_BASE_TYPES_H
#define GLDRAW_BASE_TYPES_H

/**
 * @typedef ObjectId
 * @brief Stable object identifier type within a document.
 */
typedef unsigned int ObjectId;

/**
 * @struct Vec2
 * @brief 2D vector (usable for world or screen coordinates).
 *
 * @member x X component.
 * @member y Y component.
 */
typedef struct {
    float x;
    float y;
} Vec2;

/**
 * @struct Color
 * @brief RGBA color (component range is typically `[0, 1]`).
 *
 * @member r Red component.
 * @member g Green component.
 * @member b Blue component.
 * @member a Alpha component.
 */
typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

/**
 * @struct RectF
 * @brief Floating-point rectangle (x/y/w/h).
 *
 * @member x X coordinate of the bottom-left corner (or top-left, depending on caller's coordinate system).
 * @member y Y coordinate of the bottom-left corner (or top-left, depending on caller's coordinate system).
 * @member w Width.
 * @member h Height.
 */
typedef struct {
    float x;
    float y;
    float w;
    float h;
} RectF;

#endif /* GLDRAW_BASE_TYPES_H */
