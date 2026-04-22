/**
 * @file workspace_actions.c
 * @brief Workspace-level action arbitration and unsaved-changes modal flow.
 *
 * Role in project:
 * - Decides whether destructive actions execute immediately or open a modal.
 * - Owns the pending-action state machine used by UI and application events.
 *
 * Module relationships:
 * - Sits between UI/application entry points and application-owned executors.
 * - Uses workspace callbacks for save operations before deferred execution.
 */
#include <app/workspace_actions.h>

#include <stdio.h>
#include <string.h>

/** Return non-zero when the requested action can discard the current document state. */
static int workspace_action_requires_unsaved_confirmation(WorkspaceActionType action)
{
    switch (action) {
    case WORKSPACE_ACTION_NEW_DOCUMENT:
    case WORKSPACE_ACTION_OPEN_DOCUMENT:
    case WORKSPACE_ACTION_EXIT_APPLICATION:
        return 1;
    case WORKSPACE_ACTION_NONE:
    default:
        return 0;
    }
}

/** Execute an application-owned action immediately through the workspace callback. */
static int workspace_execute_action_now(Workspace* workspace, WorkspaceActionType action)
{
    if (!workspace || action == WORKSPACE_ACTION_NONE || !workspace->execute_action) {
        return 0;
    }

    return workspace->execute_action(workspace, action, workspace->command_user_data);
}

/** Populate one reusable dialog button descriptor. */
static void workspace_set_dialog_button(UiDialogState* dialog,
                                        int index,
                                        const char* label,
                                        UiDialogResult result,
                                        int is_default)
{
    if (!dialog || !label || index < 0 || index >= (int)(sizeof(dialog->buttons) / sizeof(dialog->buttons[0]))) {
        return;
    }

    snprintf(dialog->buttons[index].label,
             sizeof(dialog->buttons[index].label),
             "%s",
             label);
    dialog->buttons[index].result = result;
    dialog->buttons[index].is_default = is_default;
}

/** Clear any active UI request/dialog state. */
static void workspace_clear_ui_request(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->active_request_type = UI_REQUEST_NONE;
    memset(&workspace->active_dialog, 0, sizeof(workspace->active_dialog));
    workspace->active_dialog.kind = UI_DIALOG_NONE;
}

/** Open a reusable workspace dialog for unsaved-changes confirmation. */
static void workspace_open_unsaved_changes_dialog(Workspace* workspace,
                                                  WorkspaceActionType action)
{
    if (!workspace) {
        return;
    }

    workspace->active_request_type = UI_REQUEST_DIALOG;
    memset(&workspace->active_dialog, 0, sizeof(workspace->active_dialog));
    workspace->active_dialog.kind = UI_DIALOG_CONFIRM_UNSAVED;
    snprintf(workspace->active_dialog.title,
             sizeof(workspace->active_dialog.title),
             "%s",
             "Unsaved Changes");
    snprintf(workspace->active_dialog.message,
             sizeof(workspace->active_dialog.message),
             "%s",
             "You have unsaved changes.\nDo you want to save before continuing?");
    workspace->active_dialog.payload.int_values[0] = (int)action;
    workspace_set_dialog_button(&workspace->active_dialog,
                                0,
                                "Save",
                                UI_DIALOG_RESULT_PRIMARY,
                                1);
    workspace_set_dialog_button(&workspace->active_dialog,
                                1,
                                "Don't Save",
                                UI_DIALOG_RESULT_SECONDARY,
                                0);
    workspace_set_dialog_button(&workspace->active_dialog,
                                2,
                                "Cancel",
                                UI_DIALOG_RESULT_CANCEL,
                                0);
    workspace->active_dialog.button_count = 3;
    workspace->active_dialog.width = 360.0f;
    workspace->active_dialog.height = 170.0f;
    workspace->active_dialog.modal = 1;
}

int workspace_modal_is_active(const Workspace* workspace)
{
    return workspace &&
           workspace->active_request_type == UI_REQUEST_DIALOG &&
           workspace->active_dialog.modal &&
           workspace->active_dialog.kind != UI_DIALOG_NONE;
}

UiDialogKind workspace_active_dialog_kind(const Workspace* workspace)
{
    if (!workspace || workspace->active_request_type != UI_REQUEST_DIALOG) {
        return UI_DIALOG_NONE;
    }
    return workspace->active_dialog.kind;
}

int workspace_request_action(Workspace* workspace, WorkspaceActionType action)
{
    if (!workspace || action == WORKSPACE_ACTION_NONE) {
        return 0;
    }

    if (workspace_modal_is_active(workspace)) {
        return 0;
    }

    if (workspace_action_requires_unsaved_confirmation(action) &&
        workspace->document_dirty) {
        workspace_open_unsaved_changes_dialog(workspace, action);
        return 1;
    }

    return workspace_execute_action_now(workspace, action);
}

int workspace_confirm_pending_action_save(Workspace* workspace)
{
    WorkspaceActionType action = WORKSPACE_ACTION_NONE;

    if (!workspace ||
        workspace_active_dialog_kind(workspace) != UI_DIALOG_CONFIRM_UNSAVED ||
        !workspace->save_document) {
        return 0;
    }

    if (!workspace->save_document(workspace, workspace->command_user_data)) {
        return 0;
    }

    action = (WorkspaceActionType)workspace->active_dialog.payload.int_values[0];
    workspace_clear_ui_request(workspace);
    return workspace_execute_action_now(workspace, action);
}

int workspace_confirm_pending_action_discard(Workspace* workspace)
{
    WorkspaceActionType action = WORKSPACE_ACTION_NONE;

    if (!workspace ||
        workspace_active_dialog_kind(workspace) != UI_DIALOG_CONFIRM_UNSAVED) {
        return 0;
    }

    action = (WorkspaceActionType)workspace->active_dialog.payload.int_values[0];
    workspace_clear_ui_request(workspace);
    return workspace_execute_action_now(workspace, action);
}

void workspace_confirm_pending_action_cancel(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace_clear_ui_request(workspace);
    snprintf(workspace->status_message,
             sizeof(workspace->status_message),
             "Action cancelled.");
}

int workspace_resolve_active_dialog(Workspace* workspace, UiDialogResult result)
{
    if (!workspace || workspace->active_request_type != UI_REQUEST_DIALOG) {
        return 0;
    }

    switch (result) {
    case UI_DIALOG_RESULT_PRIMARY:
        return workspace_confirm_pending_action_save(workspace);
    case UI_DIALOG_RESULT_SECONDARY:
        return workspace_confirm_pending_action_discard(workspace);
    case UI_DIALOG_RESULT_CANCEL:
        workspace_confirm_pending_action_cancel(workspace);
        return 1;
    case UI_DIALOG_RESULT_NONE:
    default:
        return 0;
    }
}
