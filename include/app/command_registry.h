/**
 * @file command_registry.h
 * @brief Stable editor command execution helper.
 */
#ifndef GLDRAW_APP_COMMAND_REGISTRY_H
#define GLDRAW_APP_COMMAND_REGISTRY_H

#include <app/command_types.h>

typedef struct ToolContext ToolContext;
struct Workspace;

/** Execute one command against the current editor state. */
int command_registry_execute(struct Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command);

/** Check whether a stable command has a registered execution handler. */
int command_registry_has_static_handler(EditorCommand command);

#endif /* GLDRAW_APP_COMMAND_REGISTRY_H */
