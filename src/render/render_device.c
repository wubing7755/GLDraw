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

int render_device_begin_pass(RenderDevice* device, const RenderPass* pass)
{
    if (!device || !device->vtable || !device->vtable->begin_pass) {
        return 0;
    }

    return device->vtable->begin_pass(device, pass);
}

int render_device_draw_geometry(RenderDevice* device,
                                const RenderGeometry* geometry,
                                const RenderMaterial* material)
{
    if (!device || !device->vtable || !device->vtable->draw_geometry) {
        return 0;
    }

    return device->vtable->draw_geometry(device, geometry, material);
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
