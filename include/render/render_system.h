/**
 * @file render_system.h
 * @brief OpenGL rendering system interface.
 */
#ifndef GLDRAW_RENDER_RENDER_SYSTEM_H
#define GLDRAW_RENDER_RENDER_SYSTEM_H

#include <canvas/canvas_view.h>
#include <platform/window.h>
#include <render/render_device.h>
#include <render/render_scene.h>

typedef struct RenderSystem RenderSystem;

typedef struct RenderSystemDesc {
    int logical_width;
    int logical_height;
    int framebuffer_width;
    int framebuffer_height;
    int owns_device;
} RenderSystemDesc;

/**
 * @brief Create the rendering system and initialize GPU resources.
 * @param device Render device used for all draw/resize/export operations.
 * @param desc Initial render target size and ownership policy. When
 *             owns_device is non-zero, render_system_destroy() destroys device.
 * @return Renderer instance on success, or `NULL` on failure.
 */
RenderSystem* render_system_create_with_desc(RenderDevice* device,
                                             const RenderSystemDesc* desc);

/**
 * @brief Create the rendering system from a platform window.
 * @param window Already-initialized platform window and OpenGL context.
 * @note Compatibility wrapper. The created RenderSystem owns `device`.
 * @return Renderer instance on success, or `NULL` on failure.
 */
RenderSystem* render_system_create(RenderDevice* device, const PlatformWindow* window);

/**
 * @brief Destroy the rendering system and release GPU/heap resources.
 * @param renderer Renderer instance.
 * @note Destroys the RenderDevice only when ownership was requested at create time.
 * @return No return value.
 */
void render_system_destroy(RenderSystem* renderer);

/**
 * @brief Handle window resize events.
 * @param renderer Renderer instance.
 * @param logical_width New logical window width in screen coordinates.
 * @param logical_height New logical window height in screen coordinates.
 * @param framebuffer_width New framebuffer width in pixels.
 * @param framebuffer_height New framebuffer height in pixels.
 * @return No return value.
 */
void render_system_resize(RenderSystem* renderer,
                          int logical_width,
                          int logical_height,
                          int framebuffer_width,
                          int framebuffer_height);

/**
 * @brief Draw one frame of canvas content.
 *
 * Main flow: clear -> draw grid/axes -> draw document objects -> draw tool overlay preview.
 *
 * @param renderer Renderer instance.
 * @param scene Current editor scene snapshot.
 * @return No return value.
 */
void render_system_draw(RenderSystem* renderer, const RenderSceneDesc* scene);

/**
 * @brief Export the current canvas viewport pixels from the framebuffer to a PNG file.
 * @param renderer Renderer instance.
 * @param canvas Current canvas view.
 * @param path Destination PNG path.
 * @return Non-zero on success, zero on failure.
 */
int render_system_export_png(RenderSystem* renderer,
                             const CanvasView* canvas,
                             const char* path);

#endif /* GLDRAW_RENDER_RENDER_SYSTEM_H */
