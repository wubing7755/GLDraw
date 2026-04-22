/**
 * @file ui_dialog.h
 * @brief Reusable modal dialog rendering helpers.
 */
#ifndef GLDRAW_UI_UI_DIALOG_H
#define GLDRAW_UI_UI_DIALOG_H

#include <app/workspace.h>

#ifndef NK_NUKLEAR_H_
struct nk_context;
#endif

/** Render one workspace-owned dialog and return the user result for this frame. */
UiDialogResult ui_dialog_show(struct nk_context* ctx,
                              const UiDialogState* dialog,
                              int window_width,
                              int window_height);

#endif /* GLDRAW_UI_UI_DIALOG_H */
