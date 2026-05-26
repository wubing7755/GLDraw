/**
 * @file ui_menubar.c
 * @brief Top menu bar rendering and menu interaction handling.
 *
 * Role in project:
 * - Renders top-level menus, quick action buttons, and theme menu.
 * - Converts UI clicks into menu action dispatch requests.
 *
 * Module relationships:
 * - Uses static menu definitions and action executor modules.
 * - Called by `ui_system` once per frame.
 */
#include "ui_menubar_internal.h"

#include "ui_menu_actions.h"

#include <stdlib.h>

/**
 * @brief Create a new menu bar instance.
 * @param ctx [in] Nuklear context pointer.
 * @return Newly allocated menu bar, or `NULL` on allocation failure.
 */
UiMenuBar* ui_menubar_create(void* ctx)
{
    UiMenuBar* menubar = (UiMenuBar*)calloc(1, sizeof(*menubar));
    if (!menubar) {
        return NULL;
    }

    menubar->ctx = (struct nk_context*)ctx;
    menubar->show_inspector = true;
    menubar->menu_height = MENU_BAR_HEIGHT;
    menubar->active_theme_index = 0;
    menubar->requested_theme_index = -1;
    menubar->requested_theme_reload = 0;

    return menubar;
}

/**
 * @brief Release menu bar resources.
 * @param menubar [in] Menu bar to destroy; no-op when `NULL`.
 * @return None.
 */
void ui_menubar_destroy(UiMenuBar* menubar)
{
    free(menubar);
}

/**
 * @brief Query whether the inspector panel is visible.
 * @param menubar [in] Menu bar instance; defaults to `true` when `NULL`.
 * @return `true` if inspector is visible, `false` otherwise.
 */
bool ui_menubar_inspector_visible(const UiMenuBar* menubar)
{
    return menubar ? menubar->show_inspector : true;
}

int ui_menubar_menu_open(const UiMenuBar* menubar)
{
    return menubar ? menubar->menu_open : 0;
}

/**
 * @brief Get the current menu bar height.
 * @param menubar [in] Menu bar instance; defaults to `MENU_BAR_HEIGHT` when `NULL`.
 * @return Menu bar height in pixels.
 */
float ui_menubar_height(const UiMenuBar* menubar)
{
    return menubar ? menubar->menu_height : MENU_BAR_HEIGHT;
}

/**
 * @brief Set the menu bar height.
 * @param menubar [in,out] Menu bar instance; no-op when `NULL`.
 * @param height [in] New height in pixels; minimum enforced to `MENU_BAR_HEIGHT`.
 * @return None.
 */
void ui_menubar_set_height(UiMenuBar* menubar, float height)
{
    if (!menubar) {
        return;
    }

    menubar->menu_height = (height > 10.0f) ? height : MENU_BAR_HEIGHT;
}

/**
 * @brief Assign the available theme registry to the menu bar.
 * @param menubar [in,out] Menu bar instance; no-op when `NULL`.
 * @param themes [in] Array of theme descriptors; may be `NULL` when `theme_count` is zero.
 * @param theme_count [in] Number of themes in `themes` array; treated as zero when negative.
 * @return None.
 */
void ui_menubar_set_themes(UiMenuBar* menubar, const UiThemeDescriptor* themes, int theme_count)
{
    if (!menubar) {
        return;
    }

    menubar->themes = themes;
    menubar->theme_count = (theme_count > 0) ? theme_count : 0;

    if (menubar->theme_count == 0) {
        menubar->active_theme_index = -1;
    } else if (menubar->active_theme_index < 0 ||
               menubar->active_theme_index >= menubar->theme_count) {
        menubar->active_theme_index = 0;
    }
}

/**
 * @brief Set the active theme by index.
 * @param menubar [in,out] Menu bar instance; no-op when `NULL`.
 * @param theme_index [in] Theme index to activate; clamped to valid range.
 * @return None.
 */
void ui_menubar_set_active_theme_index(UiMenuBar* menubar, int theme_index)
{
    if (!menubar) {
        return;
    }

    if (menubar->theme_count <= 0) {
        menubar->active_theme_index = -1;
        return;
    }

    if (theme_index < 0) {
        menubar->active_theme_index = 0;
        return;
    }
    if (theme_index >= menubar->theme_count) {
        menubar->active_theme_index = menubar->theme_count - 1;
        return;
    }

    menubar->active_theme_index = theme_index;
}

/**
 * @brief Atomically consume any pending theme change request.
 * @param menubar [in,out] Menu bar instance; returns `-1` when `NULL`.
 * @return Requested theme index, or `-1` if no request is pending.
 */
int ui_menubar_take_theme_request(UiMenuBar* menubar)
{
    int requested_theme_index = -1;

    if (!menubar) {
        return -1;
    }

    requested_theme_index = menubar->requested_theme_index;
    menubar->requested_theme_index = -1;
    return requested_theme_index;
}

/**
 * @brief Atomically consume any pending theme reload request.
 * @param menubar [in,out] Menu bar instance; returns `0` when `NULL`.
 * @return `1` if reload was requested, `0` otherwise.
 */
int ui_menubar_take_theme_reload_request(UiMenuBar* menubar)
{
    int requested = 0;

    if (!menubar) {
        return 0;
    }

    requested = menubar->requested_theme_reload;
    menubar->requested_theme_reload = 0;
    return requested;
}

/**
 * @brief Dispatch the clicked menu action.
 * @param menubar Menu bar instance.
 * @param workspace Workspace instance.
 * @param clicked_id Clicked menu action id.
 * @return None.
 */
static void ui_dispatch_menu_action(UiMenuBar* menubar,
                                    const EditorActionSink* sink,
                                    int clicked_id)
{
    if (!menubar || clicked_id == -1) {
        return;
    }

    if (clicked_id == MENU_ID_VIEW_TOGGLE_INSPECTOR) {
        menubar->show_inspector = !menubar->show_inspector;
        return;
    }

    ui_menu_execute(sink, (MenuId)clicked_id);
}

void ui_menubar_build(UiMenuBar* menubar,
                      const EditorViewModel* view_model,
                      const EditorActionSink* sink,
                      int window_width)
{
    int clicked_id = -1;

    if (!menubar || !view_model) {
        return;
    }

    if (!menubar->ctx) {
        return;
    }
    menubar->menu_open = 0;

    clicked_id = ui_menubar_render_contents(menubar, view_model, window_width);
    ui_dispatch_menu_action(menubar, sink, clicked_id);
}
