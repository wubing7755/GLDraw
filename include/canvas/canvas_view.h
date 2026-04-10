#ifndef GLDRAW_CANVAS_CANVAS_VIEW_H
#define GLDRAW_CANVAS_CANVAS_VIEW_H

#include <base/types.h>
#include <document/document.h>

typedef struct CanvasView {
    Document* document;
    RectF viewport;
    Vec2 center;
    float zoom;
    int show_grid;
    Color background;
} CanvasView;

void canvas_view_init(CanvasView* canvas, Document* document, RectF viewport);
void canvas_view_set_document(CanvasView* canvas, Document* document);
void canvas_view_set_viewport(CanvasView* canvas, RectF viewport);
Vec2 canvas_view_world_to_screen(const CanvasView* canvas, Vec2 world);
Vec2 canvas_view_screen_to_world(const CanvasView* canvas, Vec2 screen);
void canvas_view_pan_screen_delta(CanvasView* canvas, Vec2 delta_screen);
void canvas_view_zoom_at_screen_point(CanvasView* canvas, float factor, Vec2 screen_anchor);
float canvas_view_world_tolerance_for_pixels(const CanvasView* canvas, float pixels);
RectF canvas_view_visible_world_rect(const CanvasView* canvas);
GraphicObject* canvas_view_pick_object(const CanvasView* canvas, Vec2 screen_point, float tolerance_pixels);

#endif /* GLDRAW_CANVAS_CANVAS_VIEW_H */
