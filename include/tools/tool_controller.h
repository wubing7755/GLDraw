#ifndef GLDRAW_TOOLS_TOOL_CONTROLLER_H
#define GLDRAW_TOOLS_TOOL_CONTROLLER_H

#include <tools/tool.h>

typedef struct {
    Tool tools[TOOL_KIND_COUNT];
    ToolKind active_kind;
    int pointer_captured;
    Vec2 last_screen;
    Vec2 last_world;
} ToolController;

void tool_controller_init(ToolController* controller);
void tool_controller_shutdown(ToolController* controller);

void tool_controller_set_active(ToolController* controller, ToolContext* context, ToolKind kind);
Tool* tool_controller_get_active(ToolController* controller);
const char* tool_controller_active_label(const ToolController* controller);
GraphicObject* tool_controller_overlay_object(const ToolController* controller);

void tool_controller_pointer_down(ToolController* controller, ToolContext* context, const ToolEvent* event);
void tool_controller_pointer_move(ToolController* controller, ToolContext* context, const ToolEvent* event);
void tool_controller_pointer_up(ToolController* controller, ToolContext* context, const ToolEvent* event);
void tool_controller_key_down(ToolController* controller, ToolContext* context, int key, int mods);
void tool_controller_scroll(ToolController* controller, ToolContext* context, Vec2 screen_pos, float yoffset);

#endif /* GLDRAW_TOOLS_TOOL_CONTROLLER_H */
