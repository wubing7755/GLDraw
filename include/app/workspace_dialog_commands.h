/**
 * @file workspace_dialog_commands.h
 * @brief Workspace-owned help and modal dialog command helpers.
 */
#ifndef GLDRAW_APP_WORKSPACE_DIALOG_COMMANDS_H
#define GLDRAW_APP_WORKSPACE_DIALOG_COMMANDS_H

#include <app/workspace_service_types.h>

/** Toggle the keyboard shortcuts help dialog. */
int workspace_dialog_command_toggle_shortcuts(Workspace* workspace);
/** Open the About information dialog. */
int workspace_dialog_command_open_about(Workspace* workspace);
/** Confirm the active modal dialog when supported. */
int workspace_dialog_command_confirm(Workspace* workspace);
/** Cancel the active modal dialog when supported. */
int workspace_dialog_command_cancel(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_DIALOG_COMMANDS_H */
