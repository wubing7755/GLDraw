#ifndef GLDRAW_RENDER_RENDER_SYSTEM_H
#define GLDRAW_RENDER_RENDER_SYSTEM_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <platform/window.h>

typedef struct RenderSystem RenderSystem;

RenderSystem* render_system_create(PlatformWindow* window);
void render_system_destroy(RenderSystem* renderer);
void render_system_resize(RenderSystem* renderer, int width, int height);
void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const CanvasView* canvas,
                        const GraphicObject* overlay_object);

#endif /* GLDRAW_RENDER_RENDER_SYSTEM_H */
