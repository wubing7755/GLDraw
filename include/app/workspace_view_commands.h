/**
 * @file workspace_view_commands.h
 * @brief Workspace view command operations.
 */
#ifndef GLDRAW_APP_WORKSPACE_VIEW_COMMANDS_H
#define GLDRAW_APP_WORKSPACE_VIEW_COMMANDS_H

#include <app/workspace_service_types.h>

int workspace_view_zoom_in(Workspace* workspace);
int workspace_view_zoom_out(Workspace* workspace);
int workspace_view_zoom_to_fit(Workspace* workspace);
int workspace_view_toggle_grid(Workspace* workspace);
int workspace_view_toggle_inspector(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_VIEW_COMMANDS_H */
