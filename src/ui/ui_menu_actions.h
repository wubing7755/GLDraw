/**
 * @file ui_menu_actions.h
 * @brief Menu action dispatch interface.
 */
#ifndef GLDRAW_UI_UI_MENU_ACTIONS_H
#define GLDRAW_UI_UI_MENU_ACTIONS_H

#include "ui_menu_def.h"
#include <ui/editor_action.h>
#include <ui/editor_viewmodel.h>

/**
 * @brief Emits the specified menu action into the editor action sink.
 * @param sink Action sink.
 * @param id Menu action ID.
 * @return None.
 */
void ui_menu_execute(const EditorActionSink* sink, MenuId id);

/**
 * @brief Checks if menu action is currently available.
 * @param view_model Read-only editor view model.
 * @param id Menu action ID.
 * @return Non-zero if available, 0 if unavailable.
 */
int ui_menu_is_action_available(const EditorViewModel* view_model, MenuId id);

#endif /* GLDRAW_UI_UI_MENU_ACTIONS_H */
