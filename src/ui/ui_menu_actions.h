/**
 * @file ui_menu_actions.h
 * @brief 菜单动作分发接口。
 */
#ifndef GLDRAW_UI_UI_MENU_ACTIONS_H
#define GLDRAW_UI_UI_MENU_ACTIONS_H

#include "ui_menu_def.h"

struct Workspace;

/**
 * @brief 执行指定菜单动作。
 * @param workspace 工作区对象。
 * @param id 菜单动作 ID。
 * @return 无。
 */
void ui_menu_execute(struct Workspace* workspace, MenuId id);

/**
 * @brief 判断菜单动作当前是否可用。
 * @param id 菜单动作 ID。
 * @return 可用返回非零，不可用返回 0。
 */
int ui_menu_is_action_available(MenuId id);

#endif /* GLDRAW_UI_UI_MENU_ACTIONS_H */
