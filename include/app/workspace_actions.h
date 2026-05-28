/**
 * @file workspace_actions.h
 * @brief Workspace-level command arbitration and modal resolution helpers.
 *
 * Role in project:
 * - Centralizes destructive document actions behind one request/confirm flow.
 * - Keeps unsaved-change policy out of menu/UI/platform event call sites.
 *
 * Module relationships:
 * - Called by application event handlers and menu actions.
 * - Consumed by UI modal rendering to resolve pending actions.
 */
#ifndef GLDRAW_APP_WORKSPACE_ACTIONS_H
#define GLDRAW_APP_WORKSPACE_ACTIONS_H

#include <app/ui_dialog_types.h>
#include <app/workspace_service_types.h>

/** Return non-zero when a workspace modal is currently active. */
int workspace_modal_is_active(const Workspace* workspace);
/** Return the active dialog kind, or `UI_DIALOG_NONE` when none. */
UiDialogKind workspace_active_dialog_kind(const Workspace* workspace);
/** Resolve the active dialog using a generic dialog result. */
int workspace_resolve_active_dialog(Workspace* workspace, UiDialogResult result);

/** Request a workspace action, deferring through unsaved confirmation when needed. */
int workspace_request_action(Workspace* workspace, WorkspaceActionType action);
/** Resolve current modal by saving first, then continuing pending action on success. */
int workspace_confirm_pending_action_save(Workspace* workspace);
/** Resolve current modal by discarding changes and continuing pending action. */
int workspace_confirm_pending_action_discard(Workspace* workspace);
/** Cancel the active modal and clear the pending action. */
void workspace_confirm_pending_action_cancel(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_ACTIONS_H */
