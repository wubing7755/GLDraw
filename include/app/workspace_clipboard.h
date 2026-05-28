/**
 * @file workspace_clipboard.h
 * @brief Workspace clipboard edit operations.
 */
#ifndef GLDRAW_APP_WORKSPACE_CLIPBOARD_H
#define GLDRAW_APP_WORKSPACE_CLIPBOARD_H

#include <app/workspace_service_types.h>

/** Copy the current selection into the workspace clipboard. */
int workspace_clipboard_copy_selection(Workspace* workspace);

/** Paste the current workspace clipboard into the active layer. */
int workspace_clipboard_paste(Workspace* workspace);

/** Cut the current editable selection into the workspace clipboard. */
int workspace_clipboard_cut_selection(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_CLIPBOARD_H */
