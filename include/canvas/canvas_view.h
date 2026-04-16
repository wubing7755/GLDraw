/**
 * @file canvas_view.h
 * @brief Canvas camera and coordinate conversion API.
 *
 * Role in project:
 * - Stores viewport, center, zoom, and presentation flags.
 * - Converts between world space and screen space for tools/rendering.
 *
 * Module relationships:
 * - Reads `Document` for picking.
 * - Used by tools, renderer, and application event handling.
 */
#ifndef GLDRAW_CANVAS_CANVAS_VIEW_H
#define GLDRAW_CANVAS_CANVAS_VIEW_H

#include <base/types.h>
#include <document/document.h>

/** Canonical camera state used for compatibility migration. */
typedef struct CanvasViewportState {
    RectF viewport;
    Vec2 center;
    float zoom;
} CanvasViewportState;

/** Runtime canvas view state. */
typedef struct CanvasView {
    Document* document;
    CanvasViewportState viewport_state;
    RectF viewport;
    Vec2 center;
    float zoom;
    int show_grid;
    Color background;
} CanvasView;

/** Initialize canvas state and bind a document. Complexity: `O(1)`. */
void canvas_view_init(CanvasView* canvas, Document* document, RectF viewport);
/** Rebind document used for picking. Complexity: `O(1)`. */
void canvas_view_set_document(CanvasView* canvas, Document* document);
/** Update screen viewport rectangle. Complexity: `O(1)`. */
void canvas_view_set_viewport(CanvasView* canvas, RectF viewport);
/** Set world-space center. Complexity: `O(1)`. */
void canvas_view_set_center(CanvasView* canvas, Vec2 center);
/** Set zoom (clamped to valid range). Complexity: `O(1)`. */
void canvas_view_set_zoom(CanvasView* canvas, float zoom);
/** Set center and zoom atomically. Complexity: `O(1)`. */
void canvas_view_set_center_zoom(CanvasView* canvas, Vec2 center, float zoom);
/** Get current viewport rectangle. Complexity: `O(1)`. */
RectF canvas_view_viewport(const CanvasView* canvas);
/** Get current world-space center. Complexity: `O(1)`. */
Vec2 canvas_view_center(const CanvasView* canvas);
/** Get current zoom factor. Complexity: `O(1)`. */
float canvas_view_zoom(const CanvasView* canvas);
/** Convert world coordinates to screen coordinates. Complexity: `O(1)`. */
Vec2 canvas_view_world_to_screen(const CanvasView* canvas, Vec2 world);
/** Convert screen coordinates to world coordinates. Complexity: `O(1)`. */
Vec2 canvas_view_screen_to_world(const CanvasView* canvas, Vec2 screen);
/** Pan camera by a screen-space delta. Complexity: `O(1)`. */
void canvas_view_pan_screen_delta(CanvasView* canvas, Vec2 delta_screen);
/** Zoom around a fixed screen anchor to keep cursor focus stable. Complexity: `O(1)`. */
void canvas_view_zoom_at_screen_point(CanvasView* canvas, float factor, Vec2 screen_anchor);
/** Convert pixel tolerance to world-space tolerance at current zoom. Complexity: `O(1)`. */
float canvas_view_world_tolerance_for_pixels(const CanvasView* canvas, float pixels);
/** Get currently visible world-space rectangle. Complexity: `O(1)`. */
RectF canvas_view_visible_world_rect(const CanvasView* canvas);
/**
 * Pick top-most object at a screen point.
 * Returns `NULL` when canvas/document is invalid or no hit is found.
 * Complexity: `O(n)` where `n` is object count (reverse scan for top-most hit).
 */
GraphicObject* canvas_view_pick_object(const CanvasView* canvas, Vec2 screen_point, float tolerance_pixels);

#endif /* GLDRAW_CANVAS_CANVAS_VIEW_H */
