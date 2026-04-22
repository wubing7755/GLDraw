/**
 * @file ui_menu_actions.h
 * @brief Menu action dispatch interface.
 */
#ifndef GLDRAW_UI_UI_MENU_ACTIONS_H
#define GLDRAW_UI_UI_MENU_ACTIONS_H

#include "ui_menu_def.h"

struct Workspace;

/**
 * @brief Executes the specified menu action.
 * @param workspace Workspace instance.
 * @param id Menu action ID.
 * @return None.
 */
void ui_menu_execute(struct Workspace* workspace, MenuId id);

/**
 * @brief Checks if menu action is currently available.
 * @param id Menu action ID.
 * @return Non-zero if available, 0 if unavailable.
 */
int ui_menu_is_action_available(MenuId id);

#endif /* GLDRAW_UI_UI_MENU_ACTIONS_H */
