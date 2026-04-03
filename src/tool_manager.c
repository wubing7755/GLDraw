#include <core/tool_manager.h>
#include <core/tool.h>

static Tool* s_current_tool = NULL;

void toolmanager_init(void)
{
    s_current_tool = NULL;
}

void toolmanager_shutdown(void)
{
    s_current_tool = NULL;
}

Tool* toolmanager_get_current(void)
{
    return s_current_tool;
}

void toolmanager_set_tool(Tool* new_tool)
{
    s_current_tool = new_tool;
}
