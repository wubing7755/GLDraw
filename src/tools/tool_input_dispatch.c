#include <tools/tool_controller.h>

#include <canvas/canvas_view.h>

void tool_controller_pointer_down(ToolController* controller,
                                  ToolContext* context,
                                  const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    int accepted = 0;

    if (!controller || !tool || !tool->descriptor || !tool->descriptor->pointer_down) {
        return;
    }

    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
    accepted = tool->descriptor->pointer_down(tool, context, event);
    controller->pointer_captured = accepted ? 1 : 0;
}

void tool_controller_pointer_move(ToolController* controller,
                                  ToolContext* context,
                                  const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    if (!controller || !tool || !tool->descriptor || !tool->descriptor->pointer_move) {
        return;
    }
    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
    tool->descriptor->pointer_move(tool, context, event);
}

void tool_controller_pointer_up(ToolController* controller,
                                ToolContext* context,
                                const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    if (!controller || !tool || !tool->descriptor || !tool->descriptor->pointer_up) {
        return;
    }
    tool->descriptor->pointer_up(tool, context, event);
    controller->pointer_captured = 0;
    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
}

void tool_controller_key_down(ToolController* controller,
                              ToolContext* context,
                              int key,
                              int mods)
{
    Tool* tool = tool_controller_get_active(controller);
    if (tool && tool->descriptor && tool->descriptor->key_down) {
        tool->descriptor->key_down(tool, context, key, mods);
    }
}

void tool_controller_scroll(ToolController* controller,
                            ToolContext* context,
                            Vec2 screen_pos,
                            float yoffset)
{
    float factor = 1.0f;
    (void)controller;

    if (!context || !context->canvas || yoffset == 0.0f) {
        return;
    }

    factor = (yoffset > 0.0f) ? 1.1f : 0.9f;
    canvas_view_zoom_at_screen_point(context->canvas, factor, screen_pos);
}
