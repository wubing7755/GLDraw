/**
 * @file editor_action_handler.h
 * @brief Execute UI/editor actions against a workspace.
 */
#ifndef GLDRAW_APP_EDITOR_ACTION_HANDLER_H
#define GLDRAW_APP_EDITOR_ACTION_HANDLER_H

#include <app/workspace_service_types.h>
#include <editor/editor_action.h>

/** Execute one editor action against the current workspace state. */
int editor_action_handler_dispatch(Workspace* workspace, const EditorAction* action);

#endif /* GLDRAW_APP_EDITOR_ACTION_HANDLER_H */
