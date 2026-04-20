/**
 * @file ui_menu_def.h
 * @brief 菜单项静态定义与菜单 ID 枚举。
 */
#ifndef GLDRAW_UI_UI_MENU_DEF_H
#define GLDRAW_UI_UI_MENU_DEF_H

/**
 * @enum MenuItemType
 * @brief 菜单项类型。
 */
typedef enum MenuItemType {
    MENU_ITEM_ACTION,
    MENU_ITEM_SEPARATOR,
    MENU_ITEM_SUBMENU,
} MenuItemType;

/**
 * @enum MenuId
 * @brief 菜单动作 ID 枚举。
 *
 * 约定分段：
 * - 1~99: File
 * - 100~199: Edit
 * - 200~299: View
 * - 300~399: Help
 */
typedef enum MenuId {
    /* File menu (1-99) */
    MENU_ID_FILE = 1,
    MENU_ID_FILE_NEW,
    MENU_ID_FILE_OPEN,
    MENU_ID_FILE_SAVE,
    MENU_ID_FILE_SAVE_AS,
    MENU_ID_FILE_EXPORT_PNG,
    MENU_ID_FILE_RECENT,
    MENU_ID_FILE_EXIT,

    /* Edit menu (100-199) */
    MENU_ID_EDIT = 100,
    MENU_ID_EDIT_UNDO,
    MENU_ID_EDIT_REDO,
    MENU_ID_EDIT_CUT,
    MENU_ID_EDIT_COPY,
    MENU_ID_EDIT_PASTE,
    MENU_ID_EDIT_DELETE,
    MENU_ID_EDIT_SELECT_ALL,

    /* View menu (200-299) */
    MENU_ID_VIEW = 200,
    MENU_ID_VIEW_ZOOM_IN,
    MENU_ID_VIEW_ZOOM_OUT,
    MENU_ID_VIEW_ZOOM_FIT,
    MENU_ID_VIEW_TOGGLE_GRID,
    MENU_ID_VIEW_TOGGLE_INSPECTOR,

    /* Help menu (300-399) */
    MENU_ID_HELP = 300,
    MENU_ID_HELP_SHORTCUTS,
    MENU_ID_HELP_ABOUT,
} MenuId;

/**
 * @struct MenuItemDef
 * @brief 菜单项配置。
 *
 * @member type 菜单项类型。
 * @member label 显示文本。
 * @member shortcut 快捷键文本。
 * @member id 菜单动作 ID。
 * @member parent_id 父级菜单 ID（顶层为 -1）。
 */
typedef struct MenuItemDef {
    MenuItemType  type;
    const char*   label;
    const char*   shortcut;
    int           id;
    int           parent_id;
} MenuItemDef;

/**
 * @brief 获取菜单定义项数量。
 * @return 菜单定义表项数量。
 */
int ui_menu_def_count(void);

/**
 * @brief 获取菜单定义表。
 * @return 指向静态菜单定义数组的只读指针。
 */
const MenuItemDef* ui_menu_def_items(void);

#endif /* GLDRAW_UI_UI_MENU_DEF_H */
