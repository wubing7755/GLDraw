#ifndef GLDRAW_RENDER_CANVAS_DRAWLIST_H
#define GLDRAW_RENDER_CANVAS_DRAWLIST_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <model/selection.h>
#include <render/render_arena.h>
#include <render/render_device.h>

#include <stddef.h>

typedef struct CanvasStrokeCommand {
    Color color;
    float line_width;
    RenderPrimitiveType primitive;
    size_t point_offset;
    int point_count;
} CanvasStrokeCommand;

typedef struct CanvasDrawList {
    Vec2* points;
    size_t point_count;
    size_t point_capacity;
    CanvasStrokeCommand* strokes;
    size_t stroke_count;
    size_t stroke_capacity;
    RenderArena scratch_arena;
    RectF clip_rect;
    Color clear_color;
} CanvasDrawList;

void canvas_drawlist_init(CanvasDrawList* draw_list);
void canvas_drawlist_reset(CanvasDrawList* draw_list);
void canvas_drawlist_shutdown(CanvasDrawList* draw_list);
int canvas_drawlist_build(CanvasDrawList* draw_list,
                          const Document* document,
                          const SelectionSet* selection,
                          const CanvasView* canvas,
                          int selection_preview_active,
                          Vec2 selection_preview_delta,
                          const GraphicObject* overlay_object);

#endif /* GLDRAW_RENDER_CANVAS_DRAWLIST_H */
