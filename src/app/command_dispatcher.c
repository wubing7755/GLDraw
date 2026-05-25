#include <app/command_dispatcher.h>

#include <app/editor_action_handler.h>

void command_dispatcher_init(CommandDispatcher* dispatcher, Workspace* workspace)
{
    if (!dispatcher) {
        return;
    }

    dispatcher->workspace = workspace;
}

int command_dispatcher_dispatch(CommandDispatcher* dispatcher, const EditorAction* action)
{
    if (!dispatcher) {
        return 0;
    }

    return editor_action_handler_dispatch(dispatcher->workspace, action);
}

void command_dispatcher_action_callback(const EditorAction* action, void* user_data)
{
    command_dispatcher_dispatch((CommandDispatcher*)user_data, action);
}
