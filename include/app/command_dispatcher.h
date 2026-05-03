#ifndef GLDRAW_APP_COMMAND_DISPATCHER_H
#define GLDRAW_APP_COMMAND_DISPATCHER_H

#include <app/workspace.h>
#include <ui/editor_action.h>

typedef struct CommandDispatcher {
    Workspace* workspace;
} CommandDispatcher;

void command_dispatcher_init(CommandDispatcher* dispatcher, Workspace* workspace);
int command_dispatcher_dispatch(CommandDispatcher* dispatcher, const EditorAction* action);
void command_dispatcher_action_callback(const EditorAction* action, void* user_data);

#endif /* GLDRAW_APP_COMMAND_DISPATCHER_H */
