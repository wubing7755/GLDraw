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

/** Clear any active modal/pending state. */
static void workspace_clear_modal_state(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->modal_type = WORKSPACE_MODAL_NONE;
    workspace->pending_action = WORKSPACE_ACTION_NONE;
}

/** Open the shared unsaved-changes confirmation modal for one pending action. */
static void workspace_open_unsaved_changes_modal(Workspace* workspace, WorkspaceActionType action)
{
    if (!workspace) {
        return;
    }

    workspace->modal_type = WORKSPACE_MODAL_CONFIRM_UNSAVED;
    workspace->pending_action = action;
}

int workspace_modal_is_active(const Workspace* workspace)
{
    return workspace && workspace->modal_type != WORKSPACE_MODAL_NONE;
}

const char* workspace_modal_title(const Workspace* workspace)
{
    if (!workspace) {
        return "";
    }

    switch (workspace->modal_type) {
    case WORKSPACE_MODAL_CONFIRM_UNSAVED:
        return "Unsaved Changes";
    case WORKSPACE_MODAL_NONE:
    default:
        return "";
    }
}

const char* workspace_modal_message(const Workspace* workspace)
{
    if (!workspace) {
        return "";
    }

    switch (workspace->modal_type) {
    case WORKSPACE_MODAL_CONFIRM_UNSAVED:
        return "You have unsaved changes.\nDo you want to save before continuing?";
    case WORKSPACE_MODAL_NONE:
    default:
        return "";
    }
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
        workspace_open_unsaved_changes_modal(workspace, action);
        return 1;
    }

    return workspace_execute_action_now(workspace, action);
}

int workspace_confirm_pending_action_save(Workspace* workspace)
{
    WorkspaceActionType action = WORKSPACE_ACTION_NONE;

    if (!workspace ||
        workspace->modal_type != WORKSPACE_MODAL_CONFIRM_UNSAVED ||
        workspace->pending_action == WORKSPACE_ACTION_NONE ||
        !workspace->save_document) {
        return 0;
    }

    if (!workspace->save_document(workspace, workspace->command_user_data)) {
        return 0;
    }

    action = workspace->pending_action;
    workspace_clear_modal_state(workspace);
    return workspace_execute_action_now(workspace, action);
}

int workspace_confirm_pending_action_discard(Workspace* workspace)
{
    WorkspaceActionType action = WORKSPACE_ACTION_NONE;

    if (!workspace ||
        workspace->modal_type != WORKSPACE_MODAL_CONFIRM_UNSAVED ||
        workspace->pending_action == WORKSPACE_ACTION_NONE) {
        return 0;
    }

    action = workspace->pending_action;
    workspace_clear_modal_state(workspace);
    return workspace_execute_action_now(workspace, action);
}

void workspace_confirm_pending_action_cancel(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace_clear_modal_state(workspace);
    snprintf(workspace->status_message,
             sizeof(workspace->status_message),
             "Action cancelled.");
}
