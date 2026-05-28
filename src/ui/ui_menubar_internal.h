/**
 * @file ui_menubar_internal.h
 * @brief Private helpers shared by menu bar implementation files.
 */
#ifndef GLDRAW_UI_UI_MENUBAR_INTERNAL_H
#define GLDRAW_UI_UI_MENUBAR_INTERNAL_H

#include "ui_menubar.h"

#define MENU_BAR_HEIGHT 30.0f

int ui_menubar_render_contents(UiMenuBar* menubar,
                               const EditorViewModel* view_model,
                               int window_width);

#endif /* GLDRAW_UI_UI_MENUBAR_INTERNAL_H */
