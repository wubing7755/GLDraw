/**
 * @file tool_controller.h
 * @brief Active tool management and registry-backed input dispatch.
 */
#ifndef GLDRAW_TOOLS_TOOL_CONTROLLER_H
#define GLDRAW_TOOLS_TOOL_CONTROLLER_H

#include <tools/tool.h>

typedef struct {
    Tool* tools;
    int tool_count;
    int active_index;
    int pointer_captured;
    Vec2 last_screen;
    Vec2 last_world;
} ToolController;

/**
 * @brief Register all built-in tool descriptors into the tool registry.
 *
 * Must be called before tool_controller_init(). After registration,
 * tool_registry_count() reflects the full set of available tools.
 * Re-registration is idempotent.
 *
 * @return Non-zero on success, zero on failure.
 */
int register_builtin_tools(void);

void tool_controller_init(ToolController* controller);
void tool_controller_shutdown(ToolController* controller);
int tool_controller_set_active(ToolController* controller,
                               ToolContext* context,
                               const char* tool_id);
Tool* tool_controller_get_active(ToolController* controller);
const char* tool_controller_active_id(const ToolController* controller);
const char* tool_controller_active_label(const ToolController* controller);
GraphicObject* tool_controller_overlay_object(ToolController* controller,
                                              ToolContext* context);
int tool_controller_tool_count(const ToolController* controller);
const ToolDescriptor* tool_controller_tool_descriptor_at(const ToolController* controller,
                                                         int index);
void tool_controller_pointer_down(ToolController* controller,
                                  ToolContext* context,
                                  const ToolEvent* event);
void tool_controller_pointer_move(ToolController* controller,
                                  ToolContext* context,
                                  const ToolEvent* event);
void tool_controller_pointer_up(ToolController* controller,
                                ToolContext* context,
                                const ToolEvent* event);
void tool_controller_key_down(ToolController* controller,
                              ToolContext* context,
                              int key,
                              int mods);
void tool_controller_scroll(ToolController* controller,
                            ToolContext* context,
                            Vec2 screen_pos,
                            float yoffset);

#endif /* GLDRAW_TOOLS_TOOL_CONTROLLER_H */
