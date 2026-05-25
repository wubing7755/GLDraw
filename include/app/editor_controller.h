/**
 * @file editor_controller.h
 * @brief Workspace-level editor runtime facade.
 */
#ifndef GLDRAW_APP_EDITOR_CONTROLLER_H
#define GLDRAW_APP_EDITOR_CONTROLLER_H

#include <app/workspace.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <model/selection.h>
#include <tools/tool.h>

typedef struct EditorRenderScene {
    const Document* document;
    const SelectionSet* selection;
    const CanvasView* canvas;
    int selection_preview_active;
    Vec2 selection_preview_delta;
    const GraphicObject* overlay_object;
} EditorRenderScene;

ToolContext editor_controller_tool_context(Workspace* workspace);
int editor_controller_pointer_captured(const Workspace* workspace);
void editor_controller_sync_pointer_anchor(Workspace* workspace, Vec2 screen_pos);
ToolEvent editor_controller_make_tool_event(Workspace* workspace,
                                            Vec2 cursor_screen,
                                            int button,
                                            int mods,
                                            float wheel_y);
void editor_controller_pointer_move(Workspace* workspace,
                                    ToolContext* context,
                                    const ToolEvent* event);
void editor_controller_pointer_down(Workspace* workspace,
                                    ToolContext* context,
                                    const ToolEvent* event);
void editor_controller_pointer_up(Workspace* workspace,
                                  ToolContext* context,
                                  const ToolEvent* event);
void editor_controller_scroll(Workspace* workspace,
                              ToolContext* context,
                              Vec2 screen_pos,
                              float yoffset);
void editor_controller_set_canvas_viewport(Workspace* workspace, RectF viewport);
void editor_controller_set_canvas_background(Workspace* workspace, Color background);
RectF editor_controller_canvas_content_bounds(const Workspace* workspace);
const CanvasView* editor_controller_canvas(const Workspace* workspace);
int editor_controller_render_scene(Workspace* workspace, EditorRenderScene* out_scene);

#endif /* GLDRAW_APP_EDITOR_CONTROLLER_H */
