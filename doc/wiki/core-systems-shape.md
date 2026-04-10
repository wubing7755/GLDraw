# Shape System

This page keeps the old filename for link stability, but the current code uses a **graphic object model** rather than the previous `ShapeManager` architecture.

## GraphicObject

Every drawable item is a `GraphicObject` with:

- `ObjectId`
- `GraphicObjectType`
- `GraphicStyle`
- revision counter
- a concrete implementation payload
- a vtable for behavior

## VTable Responsibilities

Each object type implements:

- destroy
- translate
- bounds
- hit test
- path point count
- path point writing
- scalar property get/set

## Built-in Object Types

### Line

- stores two endpoints
- supports direct editing of `x1`, `y1`, `x2`, `y2`

### Rectangle

- stores a `RectF`
- supports editing of `x`, `y`, `width`, `height`

### Ellipse

- stores ellipse bounds as a `RectF`
- exposes the same scalar bounds properties as rectangle

## Why This Model

Compared with the removed `ShapeRegistry + ShapeManager` flow, the current model is simpler:

- object identity is stable inside the document
- selection works through ids
- rendering reads path geometry directly from the object
- tools can create temporary overlay objects using the same object API
