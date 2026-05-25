/**
 * @file editor_action_handler.c
 * @brief EditorAction execution rules for workspace state.
 */
#include <app/editor_action_handler.h>

#include <app/command_registry.h>
#include <app/workspace_actions.h>
#include <app/workspace_dialogs.h>
#include <app/workspace.h>
#include <commands/command.h>
#include <tools/tool_controller.h>

static void editor_action_handler_prune_selection(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    SelectionSet* selection = workspace_get_selection(workspace);
    int i = 0;

    if (!document || !selection) {
        return;
    }

    while (i < selection->count) {
        ObjectId id = selection->ids[i];
        if (!document_find_object(document, id) ||
            document_object_is_locked(document, id)) {
            selection_set_remove(selection, id);
            continue;
        }
        ++i;
    }
}

static int editor_action_handler_execute_command(Workspace* workspace, Command* command)
{
    Document* document = workspace_get_document(workspace);
    CommandExecutor* executor = workspace_get_command_executor(workspace);

    if (!document || !executor || !command) {
        return 0;
    }

    if (!command_executor_execute(executor, command, document)) {
        return 0;
    }

    workspace_sync_document_dirty(workspace);
    return 1;
}

static void editor_action_handler_set_status(Workspace* workspace, const char* message)
{
    workspace_set_status_message(workspace, message);
}

static ToolContext editor_action_handler_tool_context(Workspace* workspace)
{
    return workspace_tool_context(workspace);
}

int editor_action_handler_dispatch(Workspace* workspace, const EditorAction* action)
{
    if (!workspace || !action) {
        return 0;
    }

    switch (action->type) {
    case EDITOR_ACTION_EXECUTE_COMMAND:
    {
        ToolContext context = editor_action_handler_tool_context(workspace);
        return command_registry_execute(workspace,
                                        &context,
                                        action->payload.execute_command.command);
    }
    case EDITOR_ACTION_SET_TOOL:
    {
        ToolContext context = editor_action_handler_tool_context(workspace);
        ToolController* tools = workspace_get_tool_controller(workspace);
        return tools &&
               tool_controller_set_active(tools,
                                          &context,
                                          action->payload.set_tool.tool_id);
    }
    case EDITOR_ACTION_MODIFY_PROPERTY:
    {
        Document* document = workspace_get_document(workspace);
        CommandExecuteCheck check = COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
        Command* command = command_create_set_property_from_document(
            document,
            action->payload.modify_property.object_id,
            action->payload.modify_property.property_name,
            action->payload.modify_property.value);

        if (!command) {
            editor_action_handler_set_status(workspace,
                                             "Property edit could not be prepared.");
            return 0;
        }
        check = command_check_execute(command, document);
        if (check != COMMAND_EXECUTE_CHECK_OK) {
            editor_action_handler_set_status(workspace,
                                             command_execute_check_message(check));
            command->vtable->destroy(command);
            return 0;
        }
        if (!editor_action_handler_execute_command(workspace, command)) {
            editor_action_handler_set_status(workspace,
                                             "Property edit failed during execution.");
            return 0;
        }

        return 1;
    }
    case EDITOR_ACTION_SET_ACTIVE_LAYER:
    {
        Document* document = workspace_get_document(workspace);
        Command* command = command_create_set_active_layer(
            document,
            action->payload.set_active_layer.layer_id);
        if (!editor_action_handler_execute_command(workspace, command)) {
            return 0;
        }
        editor_action_handler_prune_selection(workspace);
        return 1;
    }
    case EDITOR_ACTION_SET_LAYER_VISIBILITY:
    {
        Document* document = workspace_get_document(workspace);
        Command* command = command_create_set_layer_visibility(
            document,
            action->payload.set_layer_visibility.layer_id,
            action->payload.set_layer_visibility.visible);
        if (!editor_action_handler_execute_command(workspace, command)) {
            return 0;
        }
        return 1;
    }
    case EDITOR_ACTION_SET_LAYER_LOCKED:
    {
        Document* document = workspace_get_document(workspace);
        Command* command = command_create_set_layer_locked(
            document,
            action->payload.set_layer_locked.layer_id,
            action->payload.set_layer_locked.locked);
        if (!editor_action_handler_execute_command(workspace, command)) {
            return 0;
        }
        editor_action_handler_prune_selection(workspace);
        return 1;
    }
    case EDITOR_ACTION_RENAME_LAYER:
    {
        Document* document = workspace_get_document(workspace);
        Command* command = command_create_rename_layer(
            document,
            action->payload.rename_layer.layer_id,
            action->payload.rename_layer.name);
        if (!editor_action_handler_execute_command(workspace, command)) {
            return 0;
        }
        return 1;
    }
    case EDITOR_ACTION_MOVE_LAYER:
    {
        Document* document = workspace_get_document(workspace);
        Command* command = command_create_move_layer(
            document,
            action->payload.move_layer.layer_id,
            action->payload.move_layer.target_index);
        if (!editor_action_handler_execute_command(workspace, command)) {
            return 0;
        }
        return 1;
    }
    case EDITOR_ACTION_CREATE_LAYER:
    {
        Command* command = command_create_create_layer(
            action->payload.create_layer.name);
        if (!editor_action_handler_execute_command(workspace, command)) {
            return 0;
        }
        return 1;
    }
    case EDITOR_ACTION_RESOLVE_DIALOG:
        if (workspace_active_dialog_kind(workspace) == UI_DIALOG_SAVE_AS) {
            workspace_dialog_set_input_text(workspace,
                                            action->payload.resolve_dialog.text);
        }
        return workspace_resolve_active_dialog(workspace,
                                               action->payload.resolve_dialog.result);
    case EDITOR_ACTION_SET_STATUS_MESSAGE:
        workspace_set_status_message(workspace,
                                     action->payload.set_status_message.message);
        return 1;
    case EDITOR_ACTION_NONE:
    default:
        return 0;
    }
}
