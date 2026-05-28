/**
 * @file editor_controller.c
 * @brief Workspace-level facade for tool input and render scene state.
 */
#include <app/editor_controller.h>

#include <app/workspace_internal.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <tools/tool_controller.h>

ToolContext editor_controller_tool_context(Workspace* workspace)
{
    return workspace_tool_context(workspace);
}

int editor_controller_pointer_captured(const Workspace* workspace)
{
    const ToolController* tools = workspace_get_tool_controller_const(workspace);
    return tools ? tool_controller_is_pointer_captured(tools) : 0;
}

void editor_controller_sync_pointer_anchor(Workspace* workspace, Vec2 screen_pos)
{
    CanvasView* canvas = workspace_get_canvas(workspace);
    ToolController* tools = workspace_get_tool_controller(workspace);

    if (!canvas || !tools) {
        return;
    }

    tool_controller_set_pointer_anchor(
        tools,
        screen_pos,
        canvas_view_screen_to_world(canvas, screen_pos));
}

ToolEvent editor_controller_make_tool_event(Workspace* workspace,
                                            Vec2 cursor_screen,
                                            int button,
                                            int mods,
                                            float wheel_y)
{
    CanvasView* canvas = workspace_get_canvas(workspace);
    ToolController* tools = workspace_get_tool_controller(workspace);
    ToolEvent event;
    Vec2 last_screen = vec2_make(0.0f, 0.0f);
    Vec2 last_world = vec2_make(0.0f, 0.0f);

    if (tools) {
        last_screen = tool_controller_last_screen(tools);
        last_world = tool_controller_last_world(tools);
    }

    event.screen_pos = cursor_screen;
    event.world_pos = canvas ? canvas_view_screen_to_world(canvas, cursor_screen)
                             : vec2_make(0.0f, 0.0f);
    event.delta_screen = vec2_sub(event.screen_pos, last_screen);
    event.delta_world = vec2_sub(event.world_pos, last_world);
    event.button = button;
    event.mods = mods;
    event.wheel_y = wheel_y;
    return event;
}

void editor_controller_pointer_move(Workspace* workspace,
                                    ToolContext* context,
                                    const ToolEvent* event)
{
    ToolController* tools = workspace_get_tool_controller(workspace);

    if (!tools || !event) {
        return;
    }

    tool_controller_pointer_move(tools, context, event);
}

void editor_controller_pointer_down(Workspace* workspace,
                                    ToolContext* context,
                                    const ToolEvent* event)
{
    ToolController* tools = workspace_get_tool_controller(workspace);

    if (!tools || !event) {
        return;
    }

    tool_controller_pointer_down(tools, context, event);
}

void editor_controller_pointer_up(Workspace* workspace,
                                  ToolContext* context,
                                  const ToolEvent* event)
{
    ToolController* tools = workspace_get_tool_controller(workspace);

    if (!tools || !event) {
        return;
    }

    tool_controller_pointer_up(tools, context, event);
}

void editor_controller_scroll(Workspace* workspace,
                              ToolContext* context,
                              Vec2 screen_pos,
                              float yoffset)
{
    ToolController* tools = workspace_get_tool_controller(workspace);

    if (!tools) {
        return;
    }

    tool_controller_scroll(tools, context, screen_pos, yoffset);
}

void editor_controller_set_canvas_viewport(Workspace* workspace, RectF viewport)
{
    CanvasView* canvas = workspace_get_canvas(workspace);

    if (!canvas) {
        return;
    }

    canvas_view_set_viewport(canvas, viewport);
}

void editor_controller_set_canvas_background(Workspace* workspace, Color background)
{
    CanvasView* canvas = workspace_get_canvas(workspace);

    if (!canvas) {
        return;
    }

    canvas->background = background;
}

RectF editor_controller_canvas_content_bounds(const Workspace* workspace)
{
    WorkspaceLayout layout = workspace_get_layout(workspace);

    if (!workspace) {
        return (RectF){0.0f, 0.0f, 0.0f, 0.0f};
    }

    return layout.canvas_content_bounds;
}

const CanvasView* editor_controller_canvas(const Workspace* workspace)
{
    return workspace_get_canvas_const(workspace);
}

int editor_controller_render_scene(Workspace* workspace, EditorRenderScene* out_scene)
{
    ToolController* tools = workspace_get_tool_controller(workspace);
    ToolContext context;

    if (!workspace || !out_scene || !tools) {
        return 0;
    }

    context = workspace_tool_context(workspace);
    out_scene->document = workspace_get_document_const(workspace);
    out_scene->selection = workspace_get_selection_const(workspace);
    out_scene->canvas = workspace_get_canvas_const(workspace);
    out_scene->selection_preview_active =
        workspace_selection_preview_active(workspace);
    out_scene->selection_preview_delta =
        workspace_selection_preview_delta(workspace);
    out_scene->overlay_object =
        tool_controller_overlay_object(tools, &context);
    return 1;
}
