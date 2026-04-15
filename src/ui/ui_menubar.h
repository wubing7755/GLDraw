#ifndef GLDRAW_UI_UI_MENUBAR_H
#define GLDRAW_UI_UI_MENUBAR_H

#include <stdbool.h>
#include <stddef.h>

struct nk_context;
struct Workspace;

/**
 * @file ui_menubar.h
 * @brief Menu bar public interface.
 *
 * The menu bar module handles rendering of the top-level
 * menu bar and dropdown menus using Nuklear.
 */

/**
 * @brief Menu bar state structure
 */
typedef struct UiMenuBar {
    struct nk_context* ctx;
    bool show_inspector;        /**< Inspector panel visibility */
    float menu_height;          /**< Height of the menu bar */
} UiMenuBar;

/**
 * @brief Create a new menu bar
 * @param ctx Nuklear context
 * @return New menu bar, or NULL on failure
 */
UiMenuBar* ui_menubar_create(void* ctx);

/**
 * @brief Destroy a menu bar
 * @param menubar Menu bar to destroy
 */
void ui_menubar_destroy(UiMenuBar* menubar);

/**
 * @brief Build and render the menu bar
 * @param menubar Menu bar instance
 * @param workspace Workspace to operate on
 * @param window_width Current window width
 */
void ui_menubar_build(UiMenuBar* menubar, struct Workspace* workspace, int window_width);

/**
 * @brief Check if inspector is visible
 * @param menubar Menu bar instance
 * @return true if inspector should be shown
 */
bool ui_menubar_inspector_visible(const UiMenuBar* menubar);

/**
 * @brief Get menu bar height
 * @param menubar Menu bar instance
 * @return Height in pixels
 */
float ui_menubar_height(const UiMenuBar* menubar);

#endif /* GLDRAW_UI_UI_MENUBAR_H */
