#include <core/app_state.h>
#include <string.h>

AppState g_app_state;

void app_state_init(GLFWwindow* window)
{
    memset(&g_app_state, 0, sizeof(g_app_state));
    g_app_state.window = window;
    sel_init(&g_app_state.selection);
}

void app_state_set_tools(Tool* draw_tool, Tool* select_tool, Tool* default_tool)
{
    g_app_state.draw_tool = draw_tool;
    g_app_state.select_tool = select_tool;
    g_app_state.current_draw_tool = draw_tool;
    g_app_state.current_tool = default_tool;
}

SelectionManager* app_state_get_selection(void)
{
    return &g_app_state.selection;
}
