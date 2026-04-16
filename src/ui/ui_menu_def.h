#ifndef GLDRAW_UI_UI_MENU_DEF_H
#define GLDRAW_UI_UI_MENU_DEF_H

/**
 * @file ui_menu_def.h
 * @brief Menu ID enumeration and menu item definitions.
 *
 * This file defines the configuration-driven menu system.
 * Menu items are defined as static data, making it easy to
 * add, remove, or modify menu entries without changing logic code.
 */

/**
 * @brief Menu item types
 */
typedef enum MenuItemType {
    MENU_ITEM_ACTION,     /**< Normal action item */
    MENU_ITEM_SEPARATOR,  /**< Separator line */
    MENU_ITEM_SUBMENU,    /**< Submenu container */
} MenuItemType;

/**
 * @brief Menu ID enumeration
 *
 * IDs are grouped by menu:
 *   1-99:   File menu
 *   100-199: Edit menu
 *   200-299: View menu
 *   300-399: Help menu
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
 * @brief Menu item definition structure
 *
 * Used to define menu items as static configuration data.
 */
typedef struct MenuItemDef {
    MenuItemType  type;       /**< Item type (action/separator/submenu) */
    const char*   label;      /**< Display text */
    const char*   shortcut;   /**< Keyboard shortcut description (e.g., "Ctrl+N") */
    int           id;         /**< Unique identifier */
    int           parent_id;  /**< Parent menu ID (-1 for top-level menus) */
} MenuItemDef;

/**
 * @brief Get the number of menu items
 * @return Number of items in the menu definition array
 */
int ui_menu_def_count(void);

/**
 * @brief Get the menu item definitions
 * @return Array of menu item definitions
 */
const MenuItemDef* ui_menu_def_items(void);

#endif /* GLDRAW_UI_UI_MENU_DEF_H */
