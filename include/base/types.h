#ifndef GLDRAW_BASE_TYPES_H
#define GLDRAW_BASE_TYPES_H

typedef unsigned int ObjectId;

typedef struct {
    float x;
    float y;
} Vec2;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} RectF;

#endif /* GLDRAW_BASE_TYPES_H */
