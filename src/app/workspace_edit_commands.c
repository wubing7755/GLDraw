/**
 * @file workspace_edit_commands.c
 * @brief Workspace-owned edit command helpers.
 */
#include <app/workspace_edit_commands.h>

#include <app/workspace_internal.h>
#include <commands/command.h>
#include <document/document.h>

static void workspace_edit_prune_noneditable_selection(Workspace* workspace)
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

int workspace_edit_undo(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    CommandExecutor* executor = workspace_get_command_executor(workspace);

    if (!document || !executor) {
        return 0;
    }

    if (!command_executor_undo(executor, document)) {
        return 0;
    }

    workspace_edit_prune_noneditable_selection(workspace);
    workspace_sync_document_dirty(workspace);
    return 1;
}

int workspace_edit_redo(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    CommandExecutor* executor = workspace_get_command_executor(workspace);

    if (!document || !executor) {
        return 0;
    }

    if (!command_executor_redo(executor, document)) {
        return 0;
    }

    workspace_edit_prune_noneditable_selection(workspace);
    workspace_sync_document_dirty(workspace);
    return 1;
}

int workspace_edit_delete_selection(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    CommandExecutor* executor = workspace_get_command_executor(workspace);
    SelectionSet* selection = workspace_get_selection(workspace);
    Command* command = NULL;

    if (!document || !executor || !selection || selection->count <= 0) {
        return 0;
    }

    workspace_edit_prune_noneditable_selection(workspace);
    if (selection->count <= 0) {
        return 0;
    }

    command = command_create_delete_selection(document, selection);
    if (!command ||
        !command_executor_execute(executor, command, document)) {
        return 0;
    }

    selection_set_clear(selection);
    workspace_sync_document_dirty(workspace);
    return 1;
}

int workspace_edit_select_all(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    SelectionSet* selection = workspace_get_selection(workspace);
    int i = 0;

    if (!document || !selection) {
        return 0;
    }

    selection_set_clear(selection);
    for (i = 0; i < document->count; ++i) {
        GraphicObject* object = document_get_object_at(document, i);
        if (object &&
            !document_layer_is_locked(document, object->layer_id)) {
            selection_set_add(selection, object->id);
        }
    }

    return 1;
}
