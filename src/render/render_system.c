#include <render/render_system.h>

#include <image/png_writer.h>
#include <render/canvas_renderer.h>

#include <math.h>
#include <stdlib.h>

typedef struct RenderSceneCacheKey {
    unsigned int document_revision;
    unsigned int overlay_revision;
    int selection_count;
    uint64_t selection_revision;
    int selection_preview_active;
    RectF viewport;
    Vec2 center;
    Vec2 selection_preview_delta;
    float zoom;
} RenderSceneCacheKey;

struct RenderSystem {
    RenderDevice* device;
    CanvasDrawList draw_list;
    int logical_width;
    int logical_height;
    int framebuffer_width;
    int framebuffer_height;
    int owns_device;
    RenderSceneCacheKey cache_key;
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

static RenderSceneCacheKey render_system_scene_cache_key(const RenderSceneDesc* scene)
{
    RenderSceneCacheKey key;

    key.document_revision = scene && scene->document ? document_revision(scene->document) : 0u;
    key.overlay_revision = scene && scene->overlay_object
                               ? scene->overlay_object->revision
                               : 0u;
    key.selection_count = scene && scene->selection ? scene->selection->count : 0;
    key.selection_revision = scene && scene->selection ? scene->selection->revision : 0u;
    key.selection_preview_active = scene ? scene->selection_preview_active : 0;
    key.selection_preview_delta = scene ? scene->selection_preview_delta
                                        : (Vec2){0.0f, 0.0f};
    key.viewport = scene && scene->canvas ? canvas_view_viewport(scene->canvas)
                                          : (RectF){0.0f, 0.0f, 0.0f, 0.0f};
    key.center = scene && scene->canvas ? canvas_view_center(scene->canvas)
                                        : (Vec2){0.0f, 0.0f};
    key.zoom = scene && scene->canvas ? canvas_view_zoom(scene->canvas) : 0.0f;
    return key;
}

static int render_system_scene_cache_key_equal(const RenderSceneCacheKey* a,
                                               const RenderSceneCacheKey* b)
{
    if (!a || !b) {
        return 0;
    }

    return a->document_revision == b->document_revision &&
           a->overlay_revision == b->overlay_revision &&
           a->selection_count == b->selection_count &&
           a->selection_revision == b->selection_revision &&
           a->selection_preview_active == b->selection_preview_active &&
           a->selection_preview_delta.x == b->selection_preview_delta.x &&
           a->selection_preview_delta.y == b->selection_preview_delta.y &&
           a->zoom == b->zoom &&
           a->center.x == b->center.x &&
           a->center.y == b->center.y &&
           a->viewport.x == b->viewport.x &&
           a->viewport.y == b->viewport.y &&
           a->viewport.w == b->viewport.w &&
           a->viewport.h == b->viewport.h;
}

RenderSystem* render_system_create_with_desc(RenderDevice* device,
                                             const RenderSystemDesc* desc)
{
    RenderSystem* renderer = NULL;

    if (!device || !desc) {
        return NULL;
    }

    renderer = (RenderSystem*)calloc(1u, sizeof(*renderer));
    if (!renderer) {
        return NULL;
    }

    renderer->device = device;
    renderer->logical_width = desc->logical_width;
    renderer->logical_height = desc->logical_height;
    renderer->framebuffer_width = desc->framebuffer_width;
    renderer->framebuffer_height = desc->framebuffer_height;
    renderer->owns_device = desc->owns_device ? 1 : 0;
    canvas_drawlist_init(&renderer->draw_list);
    return renderer;
}

RenderSystem* render_system_create(RenderDevice* device, const PlatformWindow* window)
{
    RenderSystemDesc desc;

    if (!window) {
        return NULL;
    }

    desc.logical_width = window->width;
    desc.logical_height = window->height;
    desc.framebuffer_width = window->framebuffer_width;
    desc.framebuffer_height = window->framebuffer_height;
    desc.owns_device = 1;
    return render_system_create_with_desc(device, &desc);
}

void render_system_destroy(RenderSystem* renderer)
{
    if (!renderer) {
        return;
    }

    canvas_drawlist_shutdown(&renderer->draw_list);
    if (renderer->owns_device) {
        render_device_destroy(renderer->device);
    }
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

void render_system_draw(RenderSystem* renderer, const RenderSceneDesc* scene)
{
    RenderSceneCacheKey cache_key;
    RenderFrameDesc frame_desc;
    RenderTransform transform;
    const Document* document = scene ? scene->document : NULL;
    const SelectionSet* selection = scene ? scene->selection : NULL;
    const CanvasView* canvas = scene ? scene->canvas : NULL;
    const GraphicObject* overlay_object = scene ? scene->overlay_object : NULL;
    int selection_preview_active = scene ? scene->selection_preview_active : 0;
    Vec2 selection_preview_delta = scene ? scene->selection_preview_delta
                                         : (Vec2){0.0f, 0.0f};

    if (!renderer || !document || !canvas) {
        return;
    }
    cache_key = render_system_scene_cache_key(scene);
    if (!renderer->draw_list_valid ||
        !render_system_scene_cache_key_equal(&renderer->cache_key, &cache_key)) {
        if (!canvas_drawlist_build(&renderer->draw_list,
                                   document,
                                   selection,
                                   canvas,
                                   selection_preview_active,
                                   selection_preview_delta,
                                   overlay_object)) {
            return;
        }
        renderer->cache_key = cache_key;
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
