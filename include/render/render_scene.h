/**
 * @file render_scene.h
 * @brief Editor scene input consumed by the render system.
 */
#ifndef GLDRAW_RENDER_RENDER_SCENE_H
#define GLDRAW_RENDER_RENDER_SCENE_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <model/selection.h>

typedef struct RenderSceneDesc {
    Document* document;
    const SelectionSet* selection;
    const CanvasView* canvas;
    int selection_preview_active;
    Vec2 selection_preview_delta;
    const GraphicObject* overlay_object;
} RenderSceneDesc;

#endif /* GLDRAW_RENDER_RENDER_SCENE_H */
