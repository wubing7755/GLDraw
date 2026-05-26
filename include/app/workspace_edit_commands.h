/**
 * @file workspace_edit_commands.h
 * @brief Workspace-owned edit command helpers.
 */
#ifndef GLDRAW_APP_WORKSPACE_EDIT_COMMANDS_H
#define GLDRAW_APP_WORKSPACE_EDIT_COMMANDS_H

#include <app/workspace.h>

/** Undo the latest document command and prune invalid selection entries. */
int workspace_edit_undo(Workspace* workspace);
/** Redo the latest document command and prune invalid selection entries. */
int workspace_edit_redo(Workspace* workspace);
/** Delete the editable portion of the current selection. */
int workspace_edit_delete_selection(Workspace* workspace);
/** Select every object that is currently on an editable layer. */
int workspace_edit_select_all(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_EDIT_COMMANDS_H */
