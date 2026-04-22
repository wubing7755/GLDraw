/**
 * @file canvas_view.h
 * @brief Canvas viewport state and coordinate transform interface.
 */
#ifndef GLDRAW_CANVAS_CANVAS_VIEW_H
#define GLDRAW_CANVAS_CANVAS_VIEW_H

#include <base/types.h>
#include <document/document.h>

/**
 * @struct CanvasViewportState
 * @brief Canonical state of the canvas camera.
 *
 * @member viewport Screen-space viewport rectangle.
 * @member center Viewport center in world coordinates.
 * @member zoom Zoom factor.
 */
typedef struct CanvasViewportState {
    RectF viewport;
    Vec2 center;
    float zoom;
} CanvasViewportState;

/**
 * @struct CanvasView
 * @brief Canvas runtime state.
 *
 * @member document Data document participating in picking and rendering (non-owning).
 * @member viewport_state Viewport/center/zoom state.
 * @member show_grid Whether to show the grid.
 * @member background Canvas background color.
 */
typedef struct CanvasView {
    Document* document;
    CanvasViewportState viewport_state;
    int show_grid;
    Color background;
} CanvasView;

/**
 * @brief Initialize the canvas state and bind a document.
 * @param canvas Canvas instance output address.
 * @param document Initially bound document (may be `NULL`).
 * @param viewport Initial screen viewport.
 * @return No return value.
 */
void canvas_view_init(CanvasView* canvas, Document* document, RectF viewport);

/**
 * @brief Re-bind a document.
 * @param canvas Canvas instance.
 * @param document New document pointer.
 * @return No return value.
 */
void canvas_view_set_document(CanvasView* canvas, Document* document);

/**
 * @brief Set the canvas screen viewport.
 * @param canvas Canvas instance.
 * @param viewport Viewport rectangle.
 * @return No return value.
 */
void canvas_view_set_viewport(CanvasView* canvas, RectF viewport);

/**
 * @brief Set the canvas center point (in world coordinates).
 * @param canvas Canvas instance.
 * @param center New center point.
 * @return No return value.
 */
void canvas_view_set_center(CanvasView* canvas, Vec2 center);

/**
 * @brief Set the zoom factor.
 * @param canvas Canvas instance.
 * @param zoom New zoom value (clamped to a valid range in the implementation).
 * @return No return value.
 */
void canvas_view_set_zoom(CanvasView* canvas, float zoom);

/**
 * @brief Atomically set center and zoom.
 * @param canvas Canvas instance.
 * @param center New center point.
 * @param zoom New zoom value.
 * @return No return value.
 */
void canvas_view_set_center_zoom(CanvasView* canvas, Vec2 center, float zoom);

/**
 * @brief Get the current screen viewport.
 * @param canvas Canvas instance.
 * @return Current viewport rectangle.
 */
RectF canvas_view_viewport(const CanvasView* canvas);

/**
 * @brief Get the current world-coordinate center.
 * @param canvas Canvas instance.
 * @return Current center point.
 */
Vec2 canvas_view_center(const CanvasView* canvas);

/**
 * @brief Get the current zoom value.
 * @param canvas Canvas instance.
 * @return Current zoom factor.
 */
float canvas_view_zoom(const CanvasView* canvas);

/**
 * @brief Convert world coordinates to screen coordinates.
 * @param canvas Canvas instance.
 * @param world World coordinate point.
 * @return Corresponding screen coordinate point.
 */
Vec2 canvas_view_world_to_screen(const CanvasView* canvas, Vec2 world);

/**
 * @brief Convert screen coordinates to world coordinates.
 * @param canvas Canvas instance.
 * @param screen Screen coordinate point.
 * @return Corresponding world coordinate point.
 */
Vec2 canvas_view_screen_to_world(const CanvasView* canvas, Vec2 screen);

/**
 * @brief Pan the canvas by a screen-space displacement.
 * @param canvas Canvas instance.
 * @param delta_screen Screen-space displacement delta.
 * @return No return value.
 */
void canvas_view_pan_screen_delta(CanvasView* canvas, Vec2 delta_screen);

/**
 * @brief Zoom at a given screen anchor point.
 *
 * Algorithm: compute the world coordinate of the anchor before zoom, zoom,
 * compute the new world coordinate, then correct the center by the difference
 * to achieve "zoom without cursor drift."
 *
 * @param canvas Canvas instance.
 * @param factor Zoom factor (> 1 zooms in, < 1 zooms out).
 * @param screen_anchor Screen anchor point.
 * @return No return value.
 */
void canvas_view_zoom_at_screen_point(CanvasView* canvas, float factor, Vec2 screen_anchor);

/**
 * @brief Convert pixel tolerance to world-coordinate tolerance.
 * @param canvas Canvas instance.
 * @param pixels Tolerance in pixels.
 * @return Tolerance in world coordinates.
 */
float canvas_view_world_tolerance_for_pixels(const CanvasView* canvas, float pixels);

/**
 * @brief Get the currently visible world rectangle.
 * @param canvas Canvas instance.
 * @return Rectangle covered by the viewport in world coordinates.
 */
RectF canvas_view_visible_world_rect(const CanvasView* canvas);

/**
 * @brief Pick an object at a screen point.
 *
 * Algorithm: iterate backward from the end of the object array (later-drawn objects take priority),
 * convert the pixel tolerance to world coordinates, and call the object's hit-test.
 *
 * @param canvas Canvas instance.
 * @param screen_point Screen-space point to pick.
 * @param tolerance_pixels Tolerance in pixels.
 * @return Pointer to the hit object; returns `NULL` on miss or invalid parameters.
 */
GraphicObject* canvas_view_pick_object(const CanvasView* canvas, Vec2 screen_point, float tolerance_pixels);

#endif /* GLDRAW_CANVAS_CANVAS_VIEW_H */
