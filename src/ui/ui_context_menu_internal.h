/**
 * @file ui_context_menu_internal.h
 * @brief Private context menu helpers shared by lifecycle and rendering code.
 */
#ifndef GLDRAW_UI_UI_CONTEXT_MENU_INTERNAL_H
#define GLDRAW_UI_UI_CONTEXT_MENU_INTERNAL_H

#include "ui/ui_context_menu.h"
#include "ui/ui_system_internal.h"

#define UI_CONTEXT_MENU_WIDTH 220.0f
#define UI_CONTEXT_MENU_HEIGHT 240.0f
#define UI_CONTEXT_MENU_ANCHOR_SIZE 1.0f

RectF ui_context_menu_popup_bounds(const UiContextMenuState *menu);

#endif /* GLDRAW_UI_UI_CONTEXT_MENU_INTERNAL_H */
