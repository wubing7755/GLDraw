#ifndef APP_STATE_H
#define APP_STATE_H

#include <GLFW/glfw3.h>
#include <core/selection_manager.h>

typedef struct Tool Tool;

/* Unified runtime state shared across input/tool orchestration. */
typedef struct {
    GLFWwindow* window;
    SelectionManager selection;
    Tool* draw_tool;
    Tool* select_tool;
    Tool* current_draw_tool;
    Tool* current_tool;
    int tool_mouse_active;
} AppState;

extern AppState g_app_state;

void app_state_init(GLFWwindow* window);
void app_state_set_tools(Tool* draw_tool, Tool* select_tool, Tool* default_tool);
SelectionManager* app_state_get_selection(void);

#endif /* APP_STATE_H */
