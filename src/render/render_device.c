#include <render/render_device.h>

int render_device_resize(RenderDevice* device,
                         int logical_width,
                         int logical_height,
                         int framebuffer_width,
                         int framebuffer_height)
{
    if (!device || !device->vtable || !device->vtable->resize) {
        return 0;
    }

    return device->vtable->resize(device,
                                  logical_width,
                                  logical_height,
                                  framebuffer_width,
                                  framebuffer_height);
}

int render_device_begin_frame(RenderDevice* device, const RenderFrameDesc* frame_desc)
{
    if (!device || !device->vtable || !device->vtable->begin_frame) {
        return 0;
    }

    return device->vtable->begin_frame(device, frame_desc);
}

void render_device_set_transform(RenderDevice* device, const RenderTransform* transform)
{
    if (device && device->vtable && device->vtable->set_transform) {
        device->vtable->set_transform(device, transform);
    }
}

void render_device_set_clip_rect(RenderDevice* device, RectF clip_rect)
{
    if (device && device->vtable && device->vtable->set_clip_rect) {
        device->vtable->set_clip_rect(device, clip_rect);
    }
}

void render_device_set_color(RenderDevice* device, Color color)
{
    if (device && device->vtable && device->vtable->set_color) {
        device->vtable->set_color(device, color);
    }
}

void render_device_draw_line(RenderDevice* device, Vec2 from, Vec2 to, float line_width)
{
    if (device && device->vtable && device->vtable->draw_line) {
        device->vtable->draw_line(device, from, to, line_width);
    }
}

void render_device_draw_rect(RenderDevice* device, RectF rect, float line_width)
{
    if (device && device->vtable && device->vtable->draw_rect) {
        device->vtable->draw_rect(device, rect, line_width);
    }
}

void render_device_draw_path(RenderDevice* device,
                             const Vec2* points,
                             int point_count,
                             RenderPathMode mode,
                             float line_width)
{
    if (device && device->vtable && device->vtable->draw_path) {
        device->vtable->draw_path(device, points, point_count, mode, line_width);
    }
}

int render_device_read_pixels(RenderDevice* device,
                              RectF rect,
                              unsigned char* out_rgba,
                              int stride_bytes)
{
    if (!device || !device->vtable || !device->vtable->read_pixels) {
        return 0;
    }

    return device->vtable->read_pixels(device, rect, out_rgba, stride_bytes);
}

void render_device_end_frame(RenderDevice* device)
{
    if (device && device->vtable && device->vtable->end_frame) {
        device->vtable->end_frame(device);
    }
}

void render_device_destroy(RenderDevice* device)
{
    if (device && device->vtable && device->vtable->destroy) {
        device->vtable->destroy(device);
    }
}
