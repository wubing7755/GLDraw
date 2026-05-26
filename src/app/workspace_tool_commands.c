/**
 * @file workspace_tool_commands.c
 * @brief Workspace-owned tool command helpers.
 */
#include <app/workspace_tool_commands.h>

#include <app/command_catalog.h>
#include <app/workspace.h>
#include <tools/tool_controller.h>

int workspace_tool_activate_command(Workspace* workspace,
                                    ToolContext* tool_context,
                                    EditorCommand command)
{
    const CommandDescriptor* descriptor = command_catalog_find_by_command(command);
    ToolController* tools = workspace_get_tool_controller(workspace);

    if (!tools || !tool_context || !descriptor || !descriptor->tool_id) {
        return 0;
    }

    return tool_controller_set_active(tools, tool_context, descriptor->tool_id);
}
