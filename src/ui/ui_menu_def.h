/**
 * @file ui_menu_def.h
 * @brief Menu item static definitions and menu ID enumeration.
 */
#ifndef GLDRAW_UI_UI_MENU_DEF_H
#define GLDRAW_UI_UI_MENU_DEF_H

/**
 * @enum MenuItemType
 * @brief Menu item type.
 */
typedef enum MenuItemType {
    MENU_ITEM_ACTION,
    MENU_ITEM_SEPARATOR,
    MENU_ITEM_SUBMENU,
} MenuItemType;

/**
 * @enum MenuId
 * @brief Menu action ID enumeration.
 *
 * Assigned ranges:
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
 * @brief Menu item configuration.
 *
 * @member type Menu item type.
 * @member label Display text.
 * @member shortcut Keyboard shortcut text.
 * @member id Menu action ID.
 * @member parent_id Parent menu ID (-1 for top-level).
 */
typedef struct MenuItemDef {
    MenuItemType  type;
    const char*   label;
    const char*   shortcut;
    int           id;
    int           parent_id;
} MenuItemDef;

/**
 * @brief Gets the menu definition count.
 * @return Number of menu definition entries.
 */
int ui_menu_def_count(void);

/**
 * @brief Gets the menu definition table.
 * @return Pointer to static menu definition array.
 */
const MenuItemDef* ui_menu_def_items(void);

#endif /* GLDRAW_UI_UI_MENU_DEF_H */
