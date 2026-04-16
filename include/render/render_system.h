/**
 * @file render_system.h
 * @brief OpenGL renderer interface for canvas/document drawing.
 *
 * Role in project:
 * - Owns shader program and GPU buffers.
 * - Draws grid, objects, selection emphasis, and tool overlays.
 *
 * Module relationships:
 * - Consumes `Document` geometry and `CanvasView` transforms.
 * - Called each frame by the application loop.
 */
#ifndef GLDRAW_RENDER_RENDER_SYSTEM_H
#define GLDRAW_RENDER_RENDER_SYSTEM_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <platform/window.h>

typedef struct RenderSystem RenderSystem;

/** Create renderer resources for an existing window/context. Returns `NULL` on failure. */
RenderSystem* render_system_create(PlatformWindow* window);
/** Release all GL/heap resources owned by renderer. */
void render_system_destroy(RenderSystem* renderer);
/** Update framebuffer-dependent renderer state after resize. */
void render_system_resize(RenderSystem* renderer, int width, int height);
/** Draw one frame of the canvas scene. Complexity is roughly `O(object_count + grid_lines)`. */
void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const CanvasView* canvas,
                        const GraphicObject* overlay_object);

#endif /* GLDRAW_RENDER_RENDER_SYSTEM_H */
