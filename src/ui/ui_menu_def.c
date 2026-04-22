#include "ui_menu_def.h"

/**
 * @file ui_menu_def.c
 * @brief Menu item static configuration data.
 *
 * This file contains the menu structure as static data.
 * To add/modify menu items, edit this array rather than
 * the rendering code.
 */

/**
 * @brief Menu item definitions
 *
 * Each entry defines one menu item with its type, label,
 * keyboard shortcut, ID, and parent menu ID.
 */
static MenuItemDef g_menu_items[] = {
    { MENU_ITEM_SUBMENU, "File", "", MENU_ID_FILE, -1 },

    { MENU_ITEM_ACTION, "New", "Ctrl+N", MENU_ID_FILE_NEW, MENU_ID_FILE },
    { MENU_ITEM_ACTION, "Open", "Ctrl+O", MENU_ID_FILE_OPEN, MENU_ID_FILE },
    { MENU_ITEM_ACTION, "Save", "Ctrl+S", MENU_ID_FILE_SAVE, MENU_ID_FILE },
    { MENU_ITEM_ACTION, "Save As...", "", MENU_ID_FILE_SAVE_AS, MENU_ID_FILE },

    /* Separator */
    { MENU_ITEM_SEPARATOR, "", "", -1, MENU_ID_FILE },

    { MENU_ITEM_ACTION, "Export as PNG", "", MENU_ID_FILE_EXPORT_PNG, MENU_ID_FILE },

    /* Separator */
    { MENU_ITEM_SEPARATOR, "", "", -1, MENU_ID_FILE },

    { MENU_ITEM_SUBMENU, "Recent Files", "", MENU_ID_FILE_RECENT, MENU_ID_FILE },

    /* Separator */
    { MENU_ITEM_SEPARATOR, "", "", -1, MENU_ID_FILE },

    { MENU_ITEM_ACTION, "Exit", "Alt+F4", MENU_ID_FILE_EXIT, MENU_ID_FILE },

    { MENU_ITEM_SUBMENU, "Edit", "", MENU_ID_EDIT, -1 },

    { MENU_ITEM_ACTION, "Undo", "Ctrl+Z", MENU_ID_EDIT_UNDO, MENU_ID_EDIT },
    { MENU_ITEM_ACTION, "Redo", "Ctrl+Y", MENU_ID_EDIT_REDO, MENU_ID_EDIT },

    /* Separator */
    { MENU_ITEM_SEPARATOR, "", "", -1, MENU_ID_EDIT },

    { MENU_ITEM_ACTION, "Cut", "Ctrl+X", MENU_ID_EDIT_CUT, MENU_ID_EDIT },
    { MENU_ITEM_ACTION, "Copy", "Ctrl+C", MENU_ID_EDIT_COPY, MENU_ID_EDIT },
    { MENU_ITEM_ACTION, "Paste", "Ctrl+V", MENU_ID_EDIT_PASTE, MENU_ID_EDIT },
    { MENU_ITEM_ACTION, "Delete", "Del", MENU_ID_EDIT_DELETE, MENU_ID_EDIT },

    /* Separator */
    { MENU_ITEM_SEPARATOR, "", "", -1, MENU_ID_EDIT },

    { MENU_ITEM_ACTION, "Select All", "Ctrl+A", MENU_ID_EDIT_SELECT_ALL, MENU_ID_EDIT },

    { MENU_ITEM_SUBMENU, "View", "", MENU_ID_VIEW, -1 },

    { MENU_ITEM_ACTION, "Zoom In", "Ctrl++", MENU_ID_VIEW_ZOOM_IN, MENU_ID_VIEW },
    { MENU_ITEM_ACTION, "Zoom Out", "Ctrl+-", MENU_ID_VIEW_ZOOM_OUT, MENU_ID_VIEW },
    { MENU_ITEM_ACTION, "Zoom to Fit", "Ctrl+0", MENU_ID_VIEW_ZOOM_FIT, MENU_ID_VIEW },

    /* Separator */
    { MENU_ITEM_SEPARATOR, "", "", -1, MENU_ID_VIEW },

    { MENU_ITEM_ACTION, "Toggle Grid", "", MENU_ID_VIEW_TOGGLE_GRID, MENU_ID_VIEW },
    { MENU_ITEM_ACTION, "Toggle Inspector Panel", "", MENU_ID_VIEW_TOGGLE_INSPECTOR, MENU_ID_VIEW },

    { MENU_ITEM_SUBMENU, "Help", "", MENU_ID_HELP, -1 },

    { MENU_ITEM_ACTION, "Keyboard Shortcuts", "?", MENU_ID_HELP_SHORTCUTS, MENU_ID_HELP },
    { MENU_ITEM_ACTION, "About", "", MENU_ID_HELP_ABOUT, MENU_ID_HELP },
};

/**
 * @brief Get the number of menu item definitions.
 * @return Total count of entries in the menu definition table.
 */

/**
 * @brief Gets the number of menu item definitions.
 * @param void No parameters.
 * @return Menu definition count.
 */
int ui_menu_def_count(void)
{
    return (int)(sizeof(g_menu_items) / sizeof(g_menu_items[0]));
}

/**
 * @brief Get the menu item definition table.
 * @return Pointer to the static menu item array.
 */

/**
 * @brief Gets the menu item definition table.
 * @param void No parameters.
 * @return Pointer to menu definition array.
 */
const MenuItemDef* ui_menu_def_items(void)
{
    return g_menu_items;
}
