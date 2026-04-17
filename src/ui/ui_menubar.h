#ifndef GLDRAW_UI_UI_MENUBAR_H
#define GLDRAW_UI_UI_MENUBAR_H

#include <stdbool.h>
#include <stddef.h>

struct nk_context;
struct Workspace;
struct UiThemeDescriptor;
typedef struct UiThemeDescriptor UiThemeDescriptor;

/**
 * @file ui_menubar.h
 * @brief Menu bar public interface.
 *
 * The menu bar module handles rendering of the top-level
 * menu bar and dropdown menus using Nuklear.
 */

/**
 * @brief Menu bar state structure.
 *
 * Owns Nuklear context reference, layout metrics, and pending theme request flags.
 */
typedef struct UiMenuBar {
    struct nk_context* ctx;              /**< Nuklear context (not owned) */
    bool show_inspector;                /**< Inspector panel visibility */
    float menu_height;                  /**< Height of the menu bar */
    const UiThemeDescriptor* themes;     /**< Available theme registry (not owned) */
    int theme_count;                    /**< Number of themes in registry */
    int active_theme_index;             /**< Currently active theme index */
    int requested_theme_index;          /**< Pending theme change request (-1 = none) */
    int requested_theme_reload;         /**< Non-zero when theme reload is requested */
} UiMenuBar;

/**
 * @brief Create a new menu bar instance.
 * @param ctx [in] Nuklear context pointer.
 * @return Newly allocated menu bar, or `NULL` on allocation failure.
 */
UiMenuBar* ui_menubar_create(void* ctx);

/**
 * @brief Release menu bar resources.
 * @param menubar [in] Menu bar to destroy; no-op when `NULL`.
 * @return None.
 */
void ui_menubar_destroy(UiMenuBar* menubar);

/**
 * @brief Build and render the menu bar.
 * @param menubar [in,out] Menu bar instance; no-op when `NULL`.
 * @param workspace [in,out] Workspace to operate on; used for menu action dispatch.
 * @param window_width [in] Current window width in pixels.
 * @return None.
 */
void ui_menubar_build(UiMenuBar* menubar, struct Workspace* workspace, int window_width);

/**
 * @brief Query whether the inspector panel is visible.
 * @param menubar [in] Menu bar instance; defaults to `true` when `NULL`.
 * @return `true` if inspector is visible, `false` otherwise.
 */
bool ui_menubar_inspector_visible(const UiMenuBar* menubar);

/**
 * @brief Get the current menu bar height.
 * @param menubar [in] Menu bar instance; defaults to `MENU_BAR_HEIGHT` when `NULL`.
 * @return Menu bar height in pixels.
 */
float ui_menubar_height(const UiMenuBar* menubar);

/**
 * @brief Set the menu bar height.
 * @param menubar [in,out] Menu bar instance; no-op when `NULL`.
 * @param height [in] New height in pixels; minimum enforced to `MENU_BAR_HEIGHT`.
 * @return None.
 */
void ui_menubar_set_height(UiMenuBar* menubar, float height);
void ui_menubar_set_themes(UiMenuBar* menubar, const UiThemeDescriptor* themes, int theme_count);
void ui_menubar_set_active_theme_index(UiMenuBar* menubar, int theme_index);
int ui_menubar_take_theme_request(UiMenuBar* menubar);
int ui_menubar_take_theme_reload_request(UiMenuBar* menubar);

/**
 * @brief Assign the available theme registry to the menu bar.
 * @param menubar [in,out] Menu bar instance; no-op when `NULL`.
 * @param themes [in] Array of theme descriptors; may be `NULL` when `theme_count` is zero.
 * @param theme_count [in] Number of themes in `themes` array; treated as zero when negative.
 * @return None.
 */
void ui_menubar_set_themes(UiMenuBar* menubar, const UiThemeDescriptor* themes, int theme_count);

/**
 * @brief Set the active theme by index.
 * @param menubar [in,out] Menu bar instance; no-op when `NULL`.
 * @param theme_index [in] Theme index to activate; clamped to valid range.
 * @return None.
 */
void ui_menubar_set_active_theme_index(UiMenuBar* menubar, int theme_index);

/**
 * @brief Atomically consume any pending theme change request.
 * @param menubar [in,out] Menu bar instance; returns `-1` when `NULL`.
 * @return Requested theme index, or `-1` if no request is pending.
 */
int ui_menubar_take_theme_request(UiMenuBar* menubar);

/**
 * @brief Atomically consume any pending theme reload request.
 * @param menubar [in,out] Menu bar instance; returns `0` when `NULL`.
 * @return `1` if reload was requested, `0` otherwise.
 */
int ui_menubar_take_theme_reload_request(UiMenuBar* menubar);

#endif /* GLDRAW_UI_UI_MENUBAR_H */
