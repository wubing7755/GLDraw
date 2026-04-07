#include <core/tool_manager.h>
#include <core/app_state.h>

void toolmanager_init(void)
{
    g_app_state.current_tool = NULL;
}

void toolmanager_shutdown(void)
{
    g_app_state.current_tool = NULL;
}

Tool* toolmanager_get_current(void)
{
    return g_app_state.current_tool;
}

void toolmanager_set_tool(Tool* new_tool)
{
    g_app_state.current_tool = new_tool;
}
