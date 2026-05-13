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
#include <app/workspace_internal.h>
#include <app/workspace_actions.h>

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

int workspace_modal_is_active(const Workspace* workspace)
{
    return workspace &&
           workspace->session.active_request_type == UI_REQUEST_DIALOG &&
           workspace->session.active_dialog.modal &&
           workspace->session.active_dialog.kind != UI_DIALOG_NONE;
}

UiDialogKind workspace_active_dialog_kind(const Workspace* workspace)
{
    if (!workspace || workspace->session.active_request_type != UI_REQUEST_DIALOG) {
        return UI_DIALOG_NONE;
    }
    return workspace->session.active_dialog.kind;
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
        workspace->session.document_dirty) {
        return workspace_dialog_open_confirm_unsaved(workspace, action);
    }

    if (!workspace->services.execute_action) {
        return 0;
    }

    return workspace->services.execute_action(workspace,
                                              action,
                                              workspace->services.command_user_data);
}

int workspace_confirm_pending_action_save(Workspace* workspace)
{
    if (!workspace ||
        workspace_active_dialog_kind(workspace) != UI_DIALOG_CONFIRM_UNSAVED) {
        return 0;
    }

    return workspace_dialog_resolve(workspace, UI_DIALOG_RESULT_PRIMARY);
}

int workspace_confirm_pending_action_discard(Workspace* workspace)
{
    if (!workspace ||
        workspace_active_dialog_kind(workspace) != UI_DIALOG_CONFIRM_UNSAVED) {
        return 0;
    }

    return workspace_dialog_resolve(workspace, UI_DIALOG_RESULT_SECONDARY);
}

void workspace_confirm_pending_action_cancel(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    (void)workspace_dialog_resolve(workspace, UI_DIALOG_RESULT_CANCEL);
}

int workspace_resolve_active_dialog(Workspace* workspace, UiDialogResult result)
{
    return workspace_dialog_resolve(workspace, result);
}
