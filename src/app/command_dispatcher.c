#include <app/command_dispatcher.h>

#include <app/command_registry.h>
#include <app/workspace_actions.h>
#include <commands/command.h>

#include <stdio.h>
#include <string.h>

static void command_dispatcher_prune_selection(Workspace* workspace)
{
    int i = 0;

    if (!workspace) {
        return;
    }

    while (i < workspace->session.selection.count) {
        ObjectId id = workspace->session.selection.ids[i];
        if (!document_find_object(&workspace->core.document, id) ||
            document_object_is_locked(&workspace->core.document, id)) {
            selection_set_remove(&workspace->session.selection, id);
            continue;
        }
        ++i;
    }
}

static void command_dispatcher_set_status(Workspace* workspace, const char* message)
{
    if (!workspace || !message) {
        return;
    }

    snprintf(workspace->session.status_message,
             sizeof(workspace->session.status_message),
             "%s",
             message);
}

static ToolContext command_dispatcher_tool_context(Workspace* workspace)
{
    ToolContext context;

    context.workspace = workspace;
    context.document = workspace ? &workspace->core.document : NULL;
    context.history = workspace ? &workspace->core.history : NULL;
    context.canvas = workspace ? &workspace->core.canvas : NULL;
    context.selection = workspace ? &workspace->session.selection : NULL;
    return context;
}

void command_dispatcher_init(CommandDispatcher* dispatcher, Workspace* workspace)
{
    if (!dispatcher) {
        return;
    }

    dispatcher->workspace = workspace;
}

int command_dispatcher_dispatch(CommandDispatcher* dispatcher, const EditorAction* action)
{
    Workspace* workspace = NULL;

    if (!dispatcher || !action) {
        return 0;
    }

    workspace = dispatcher->workspace;
    if (!workspace) {
        return 0;
    }

    switch (action->type) {
    case EDITOR_ACTION_EXECUTE_COMMAND:
    {
        ToolContext context = command_dispatcher_tool_context(workspace);
        return command_registry_execute(workspace,
                                        &context,
                                        action->payload.execute_command.command);
    }
    case EDITOR_ACTION_SET_TOOL:
    {
        ToolContext context = command_dispatcher_tool_context(workspace);
        return tool_controller_set_active(&workspace->core.tools,
                                          &context,
                                          action->payload.set_tool.tool_id);
    }
    case EDITOR_ACTION_MODIFY_PROPERTY:
    {
        CommandExecuteCheck check = COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
        Command* command = command_create_set_property_from_document(
            &workspace->core.document,
            action->payload.modify_property.object_id,
            action->payload.modify_property.property_name,
            action->payload.modify_property.value);

        if (!command) {
            command_dispatcher_set_status(workspace,
                                          "Property edit could not be prepared.");
            return 0;
        }
        check = command_check_execute(command, &workspace->core.document);
        if (check != COMMAND_EXECUTE_CHECK_OK) {
            command_dispatcher_set_status(workspace,
                                          command_execute_check_message(check));
            command->vtable->destroy(command);
            return 0;
        }
        if (!command_executor_execute(&workspace->core.commands,
                                      command,
                                      &workspace->core.document)) {
            command_dispatcher_set_status(workspace,
                                          "Property edit failed during execution.");
            return 0;
        }

        workspace_sync_document_dirty(workspace);
        return 1;
    }
    case EDITOR_ACTION_SET_ACTIVE_LAYER:
    {
        Command* command = command_create_set_active_layer(
            &workspace->core.document,
            action->payload.set_active_layer.layer_id);
        if (!command ||
            !command_executor_execute(&workspace->core.commands,
                                      command,
                                      &workspace->core.document)) {
            return 0;
        }
        command_dispatcher_prune_selection(workspace);
        workspace_sync_document_dirty(workspace);
        return 1;
    }
    case EDITOR_ACTION_SET_LAYER_VISIBILITY:
    {
        Command* command = command_create_set_layer_visibility(
            &workspace->core.document,
            action->payload.set_layer_visibility.layer_id,
            action->payload.set_layer_visibility.visible);
        if (!command ||
            !command_executor_execute(&workspace->core.commands,
                                      command,
                                      &workspace->core.document)) {
            return 0;
        }
        workspace_sync_document_dirty(workspace);
        return 1;
    }
    case EDITOR_ACTION_SET_LAYER_LOCKED:
    {
        Command* command = command_create_set_layer_locked(
            &workspace->core.document,
            action->payload.set_layer_locked.layer_id,
            action->payload.set_layer_locked.locked);
        if (!command ||
            !command_executor_execute(&workspace->core.commands,
                                      command,
                                      &workspace->core.document)) {
            return 0;
        }
        command_dispatcher_prune_selection(workspace);
        workspace_sync_document_dirty(workspace);
        return 1;
    }
    case EDITOR_ACTION_RENAME_LAYER:
    {
        Command* command = command_create_rename_layer(
            &workspace->core.document,
            action->payload.rename_layer.layer_id,
            action->payload.rename_layer.name);
        if (!command ||
            !command_executor_execute(&workspace->core.commands,
                                      command,
                                      &workspace->core.document)) {
            return 0;
        }
        workspace_sync_document_dirty(workspace);
        return 1;
    }
    case EDITOR_ACTION_MOVE_LAYER:
    {
        Command* command = command_create_move_layer(
            &workspace->core.document,
            action->payload.move_layer.layer_id,
            action->payload.move_layer.target_index);
        if (!command ||
            !command_executor_execute(&workspace->core.commands,
                                      command,
                                      &workspace->core.document)) {
            return 0;
        }
        workspace_sync_document_dirty(workspace);
        return 1;
    }
    case EDITOR_ACTION_CREATE_LAYER:
    {
        Command* command = command_create_create_layer(
            action->payload.create_layer.name);
        if (!command ||
            !command_executor_execute(&workspace->core.commands,
                                      command,
                                      &workspace->core.document)) {
            return 0;
        }
        workspace_sync_document_dirty(workspace);
        return 1;
    }
    case EDITOR_ACTION_RESOLVE_DIALOG:
        if (workspace->session.active_dialog.kind == UI_DIALOG_SAVE_AS) {
            snprintf(workspace->session.active_dialog.payload.text,
                     sizeof(workspace->session.active_dialog.payload.text),
                     "%s",
                     action->payload.resolve_dialog.text);
        }
        return workspace_resolve_active_dialog(workspace,
                                               action->payload.resolve_dialog.result);
    case EDITOR_ACTION_SET_STATUS_MESSAGE:
        snprintf(workspace->session.status_message,
                 sizeof(workspace->session.status_message),
                 "%s",
                 action->payload.set_status_message.message);
        return 1;
    case EDITOR_ACTION_NONE:
    default:
        return 0;
    }
}

void command_dispatcher_action_callback(const EditorAction* action, void* user_data)
{
    command_dispatcher_dispatch((CommandDispatcher*)user_data, action);
}
