#include <render/canvas_renderer.h>

static int canvas_renderer_same_color(Color a, Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static int canvas_renderer_can_merge_strokes(const CanvasStrokeCommand* a,
                                             const CanvasStrokeCommand* b)
{
    if (!a || !b) {
        return 0;
    }

    return a->primitive == b->primitive &&
           a->line_width == b->line_width &&
           canvas_renderer_same_color(a->color, b->color) &&
           a->point_offset + (size_t)a->point_count == b->point_offset;
}

int canvas_renderer_submit(RenderDevice* device,
                           const RenderFrameDesc* frame_desc,
                           const RenderTransform* transform,
                           const CanvasDrawList* draw_list)
{
    RenderPass pass;
    size_t i = 0u;

    if (!device || !frame_desc || !transform || !draw_list) {
        return 0;
    }

    if (!render_device_begin_frame(device, frame_desc)) {
        return 0;
    }

    pass.clip_rect = draw_list->clip_rect;
    pass.transform = *transform;
    if (!render_device_begin_pass(device, &pass)) {
        render_device_end_frame(device);
        return 0;
    }

    for (i = 0u; i < draw_list->stroke_count; ++i) {
        const CanvasStrokeCommand* stroke = &draw_list->strokes[i];
        RenderGeometry geometry;
        RenderMaterial material;
        size_t end_index = i + 1u;
        int batched_point_count = stroke->point_count;

        while (end_index < draw_list->stroke_count &&
               canvas_renderer_can_merge_strokes(stroke, &draw_list->strokes[end_index])) {
            batched_point_count += draw_list->strokes[end_index].point_count;
            ++end_index;
        }

        geometry.points = draw_list->points + stroke->point_offset;
        geometry.point_count = batched_point_count;
        geometry.primitive = stroke->primitive;
        material.color = stroke->color;
        material.line_width = stroke->line_width;
        if (!render_device_draw_geometry(device, &geometry, &material)) {
            render_device_end_frame(device);
            return 0;
        }
        i = end_index - 1u;
    }
    render_device_end_frame(device);
    return 1;
}
