#ifndef GLDRAW_RENDER_CANVAS_RENDERER_H
#define GLDRAW_RENDER_CANVAS_RENDERER_H

#include <render/canvas_drawlist.h>

int canvas_renderer_submit(RenderDevice* device,
                           const RenderFrameDesc* frame_desc,
                           const RenderTransform* transform,
                           const CanvasDrawList* draw_list);

#endif /* GLDRAW_RENDER_CANVAS_RENDERER_H */
