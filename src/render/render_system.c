#include <render/render_system.h>

#include <image/png_writer.h>
#include <render/canvas_renderer.h>

#include <math.h>
#include <stdlib.h>

struct RenderSystem {
    RenderDevice* device;
    CanvasDrawList draw_list;
    int logical_width;
    int logical_height;
    int framebuffer_width;
    int framebuffer_height;
    unsigned int cached_document_revision;
    unsigned int cached_overlay_revision;
    int cached_selection_count;
    int cached_selection_preview_active;
    RectF cached_viewport;
    Vec2 cached_center;
    Vec2 cached_selection_preview_delta;
    float cached_zoom;
    int draw_list_valid;
};

static RenderTransform render_system_make_transform(const RenderSystem* renderer)
{
    RenderTransform transform = {1.0f, 1.0f};

    if (!renderer) {
        return transform;
    }

    if (renderer->logical_width > 0) {
        transform.scale_x =
            (float)renderer->framebuffer_width / (float)renderer->logical_width;
    }
    if (renderer->logical_height > 0) {
        transform.scale_y =
            (float)renderer->framebuffer_height / (float)renderer->logical_height;
    }
    return transform;
}

RenderSystem* render_system_create(RenderDevice* device, const PlatformWindow* window)
{
    RenderSystem* renderer = NULL;

    if (!device || !window) {
        return NULL;
    }

    renderer = (RenderSystem*)calloc(1u, sizeof(*renderer));
    if (!renderer) {
        return NULL;
    }

    renderer->device = device;
    renderer->logical_width = window->width;
    renderer->logical_height = window->height;
    renderer->framebuffer_width = window->framebuffer_width;
    renderer->framebuffer_height = window->framebuffer_height;
    canvas_drawlist_init(&renderer->draw_list);
    return renderer;
}

void render_system_destroy(RenderSystem* renderer)
{
    if (!renderer) {
        return;
    }

    canvas_drawlist_shutdown(&renderer->draw_list);
    render_device_destroy(renderer->device);
    free(renderer);
}

void render_system_resize(RenderSystem* renderer,
                          int logical_width,
                          int logical_height,
                          int framebuffer_width,
                          int framebuffer_height)
{
    if (!renderer) {
        return;
    }

    renderer->logical_width = logical_width;
    renderer->logical_height = logical_height;
    renderer->framebuffer_width = framebuffer_width;
    renderer->framebuffer_height = framebuffer_height;
    renderer->draw_list_valid = 0;
    render_device_resize(renderer->device,
                         logical_width,
                         logical_height,
                         framebuffer_width,
                         framebuffer_height);
}

void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const SelectionSet* selection,
                        const CanvasView* canvas,
                        int selection_preview_active,
                        Vec2 selection_preview_delta,
                        const GraphicObject* overlay_object)
{
    RenderFrameDesc frame_desc;
    RenderTransform transform;

    if (!renderer || !document || !canvas) {
        return;
    }
    if (!renderer->draw_list_valid ||
        renderer->cached_document_revision != document->revision ||
        renderer->cached_overlay_revision != (overlay_object ? overlay_object->revision : 0u) ||
        renderer->cached_selection_count != (selection ? selection->count : 0) ||
        renderer->cached_selection_preview_active != selection_preview_active ||
        renderer->cached_selection_preview_delta.x != selection_preview_delta.x ||
        renderer->cached_selection_preview_delta.y != selection_preview_delta.y ||
        renderer->cached_zoom != canvas_view_zoom(canvas) ||
        renderer->cached_center.x != canvas_view_center(canvas).x ||
        renderer->cached_center.y != canvas_view_center(canvas).y ||
        renderer->cached_viewport.x != canvas_view_viewport(canvas).x ||
        renderer->cached_viewport.y != canvas_view_viewport(canvas).y ||
        renderer->cached_viewport.w != canvas_view_viewport(canvas).w ||
        renderer->cached_viewport.h != canvas_view_viewport(canvas).h) {
        if (!canvas_drawlist_build(&renderer->draw_list,
                                   document,
                                   selection,
                                   canvas,
                                   selection_preview_active,
                                   selection_preview_delta,
                                   overlay_object)) {
            return;
        }
        renderer->cached_document_revision = document->revision;
        renderer->cached_overlay_revision = overlay_object ? overlay_object->revision : 0u;
        renderer->cached_selection_count = selection ? selection->count : 0;
        renderer->cached_selection_preview_active = selection_preview_active;
        renderer->cached_selection_preview_delta = selection_preview_delta;
        renderer->cached_viewport = canvas_view_viewport(canvas);
        renderer->cached_center = canvas_view_center(canvas);
        renderer->cached_zoom = canvas_view_zoom(canvas);
        renderer->draw_list_valid = 1;
    }

    frame_desc.logical_width = renderer->logical_width;
    frame_desc.logical_height = renderer->logical_height;
    frame_desc.framebuffer_width = renderer->framebuffer_width;
    frame_desc.framebuffer_height = renderer->framebuffer_height;
    frame_desc.clear_color = renderer->draw_list.clear_color;
    transform = render_system_make_transform(renderer);

    canvas_renderer_submit(renderer->device,
                           &frame_desc,
                           &transform,
                           &renderer->draw_list);
}

int render_system_export_png(RenderSystem* renderer,
                             const CanvasView* canvas,
                             const char* path)
{
    RectF viewport;
    RenderTransform transform;
    int width = 0;
    int height = 0;
    unsigned char* pixels = NULL;
    int ok = 0;

    if (!renderer || !canvas || !path || path[0] == '\0') {
        return 0;
    }

    transform = render_system_make_transform(renderer);
    viewport = canvas_view_viewport(canvas);
    width = (int)ceilf(viewport.w * transform.scale_x);
    height = (int)ceilf(viewport.h * transform.scale_y);
    if (width <= 0 || height <= 0) {
        return 0;
    }

    pixels = (unsigned char*)malloc((size_t)width * (size_t)height * 4u);
    if (!pixels) {
        return 0;
    }

    if (render_device_read_pixels(renderer->device, viewport, pixels, width * 4)) {
        ok = png_writer_write_rgba_file(path, pixels, width, height);
    }

    free(pixels);
    return ok;
}
