/**
 * @file workspace_tool_commands.h
 * @brief Workspace-owned tool command helpers.
 */
#ifndef GLDRAW_APP_WORKSPACE_TOOL_COMMANDS_H
#define GLDRAW_APP_WORKSPACE_TOOL_COMMANDS_H

#include <app/command_types.h>

typedef struct ToolContext ToolContext;
struct Workspace;

/** Activate the registered tool represented by a dynamic tool command. */
int workspace_tool_activate_command(struct Workspace* workspace,
                                    ToolContext* tool_context,
                                    EditorCommand command);

#endif /* GLDRAW_APP_WORKSPACE_TOOL_COMMANDS_H */
