/**
 * @file render_system.h
 * @brief OpenGL rendering system interface.
 */
#ifndef GLDRAW_RENDER_RENDER_SYSTEM_H
#define GLDRAW_RENDER_RENDER_SYSTEM_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <platform/window.h>

typedef struct RenderSystem RenderSystem;

/**
 * @brief Create the rendering system and initialize GPU resources.
 * @param window Already-initialized platform window and OpenGL context.
 * @return Renderer instance on success, or `NULL` on failure.
 */
RenderSystem* render_system_create(PlatformWindow* window);

/**
 * @brief Destroy the rendering system and release GPU/heap resources.
 * @param renderer Renderer instance.
 * @return No return value.
 */
void render_system_destroy(RenderSystem* renderer);

/**
 * @brief Handle window resize events.
 * @param renderer Renderer instance.
 * @param width New width in pixels.
 * @param height New height in pixels.
 * @return No return value.
 */
void render_system_resize(RenderSystem* renderer, int width, int height);

/**
 * @brief Draw one frame of canvas content.
 *
 * Main flow: clear -> draw grid/axes -> draw document objects -> draw tool overlay preview.
 *
 * @param renderer Renderer instance.
 * @param document Current document.
 * @param canvas Current canvas view.
 * @param overlay_object Tool overlay preview object (may be `NULL`).
 * @return No return value.
 */
void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const CanvasView* canvas,
                        const GraphicObject* overlay_object);

#endif /* GLDRAW_RENDER_RENDER_SYSTEM_H */
