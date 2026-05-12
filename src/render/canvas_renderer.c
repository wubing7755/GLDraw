#include <render/canvas_renderer.h>

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

        geometry.points = draw_list->points + stroke->point_offset;
        geometry.point_count = stroke->point_count;
        geometry.primitive = stroke->primitive;
        material.color = stroke->color;
        material.line_width = stroke->line_width;
        if (!render_device_draw_geometry(device, &geometry, &material)) {
            render_device_end_frame(device);
            return 0;
        }
    }
    render_device_end_frame(device);
    return 1;
}
