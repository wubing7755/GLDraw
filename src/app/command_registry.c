/**
 * @file command_registry.c
 * @brief Stable command definitions and execution.
 */
#include <app/command_registry.h>

#include <app/command_availability.h>
#include <app/workspace_clipboard.h>
#include <app/workspace_dialog_commands.h>
#include <app/workspace_edit_commands.h>
#include <app/workspace_file_commands.h>
#include <app/workspace_service_types.h>
#include <app/workspace_tool_commands.h>
#include <app/workspace_view_commands.h>

#include "command_definitions.h"

typedef int (*CommandExecutionFn)(Workspace* workspace);

typedef struct CommandExecutionRoute {
    EditorCommand command;
    CommandExecutionFn execute;
} CommandExecutionRoute;

static const CommandExecutionRoute COMMAND_EXECUTION_ROUTES[] = {
#define GLDRAW_COMMAND_ROUTE(command, id, label, scope, menu_id, tool_id, availability, service, execute) \
    {command, execute},
    GLDRAW_STABLE_COMMANDS(GLDRAW_COMMAND_ROUTE)
#undef GLDRAW_COMMAND_ROUTE
};

static CommandExecutionFn command_registry_lookup_execution(EditorCommand command)
{
    int i = 0;

    for (i = 0; i < (int)(sizeof(COMMAND_EXECUTION_ROUTES) /
                          sizeof(COMMAND_EXECUTION_ROUTES[0])); ++i) {
        if (COMMAND_EXECUTION_ROUTES[i].command == command) {
            return COMMAND_EXECUTION_ROUTES[i].execute;
        }
    }

    return NULL;
}

int command_registry_has_static_handler(EditorCommand command)
{
    return command_registry_lookup_execution(command) != NULL;
}

int command_registry_execute(Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command)
{
    CommandExecutionFn execute = NULL;

    if (!workspace) {
        return 0;
    }
    if (!command_availability_is_available(workspace, command)) {
        return 0;
    }
    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        return workspace_tool_activate_command(workspace, tool_context, command);
    }

    execute = command_registry_lookup_execution(command);
    return execute ? execute(workspace) : 0;
}
