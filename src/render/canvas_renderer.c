#include <render/canvas_renderer.h>

int canvas_renderer_submit(RenderDevice* device,
                           const RenderFrameDesc* frame_desc,
                           const RenderTransform* transform,
                           const CanvasDrawList* draw_list)
{
    size_t i = 0u;

    if (!device || !frame_desc || !transform || !draw_list) {
        return 0;
    }

    if (!render_device_begin_frame(device, frame_desc)) {
        return 0;
    }

    render_device_set_transform(device, transform);
    render_device_set_clip_rect(device, draw_list->clip_rect);
    for (i = 0u; i < draw_list->stroke_count; ++i) {
        const CanvasStrokeCommand* stroke = &draw_list->strokes[i];

        render_device_set_color(device, stroke->color);
        render_device_draw_path(device,
                                draw_list->points + stroke->point_offset,
                                stroke->point_count,
                                stroke->mode,
                                stroke->line_width);
    }
    render_device_end_frame(device);
    return 1;
}
