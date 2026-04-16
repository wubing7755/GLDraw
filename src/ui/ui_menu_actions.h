#ifndef GLDRAW_UI_UI_MENU_ACTIONS_H
#define GLDRAW_UI_UI_MENU_ACTIONS_H

#include "ui_menu_def.h"

struct Workspace;

/**
 * @file ui_menu_actions.h
 * @brief Menu action handler interface.
 *
 * This module provides centralized menu action handling.
 * All menu item clicks are routed through ui_menu_execute()
 * which dispatches to the appropriate handler.
 */

/**
 * @brief Execute a menu action
 * @param workspace The workspace to operate on
 * @param id The menu item ID to execute
 *
 * Routes the menu ID to the appropriate handler function.
 * This is the single entry point for all menu actions.
 */
void ui_menu_execute(struct Workspace* workspace, MenuId id);

#endif /* GLDRAW_UI_UI_MENU_ACTIONS_H */
