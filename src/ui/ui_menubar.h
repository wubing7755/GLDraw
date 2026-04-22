/**
 * @file ui_menubar.h
 * @brief Top menu bar module public interface.
 */
#ifndef GLDRAW_UI_UI_MENUBAR_H
#define GLDRAW_UI_UI_MENUBAR_H

#include <stdbool.h>
#include <stddef.h>

struct nk_context;
struct Workspace;
struct UiThemeDescriptor;
typedef struct UiThemeDescriptor UiThemeDescriptor;

/**
 * @struct UiMenuBar
 * @brief Menu bar runtime state.
 *
 * @member ctx Nuklear context (non-owning).
 * @member show_inspector Inspector panel visibility.
 * @member menu_height Menu bar height.
 * @member themes Theme descriptor array (non-owning).
 * @member theme_count Theme count.
 * @member active_theme_index Currently active theme index.
 * @member requested_theme_index Pending theme index to apply (-1 means no request).
 * @member requested_theme_reload Whether a reload of external themes is requested.
 */
typedef struct UiMenuBar {
    struct nk_context* ctx;
    bool show_inspector;
    float menu_height;
    const UiThemeDescriptor* themes;
    int theme_count;
    int active_theme_index;
    int requested_theme_index;
    int requested_theme_reload;
} UiMenuBar;

/**
 * @brief Create a menu bar instance.
 * @param ctx Nuklear context pointer.
 * @return `UiMenuBar*` on success, `NULL` on failure.
 */
UiMenuBar* ui_menubar_create(void* ctx);

/**
 * @brief Destroy a menu bar instance.
 * @param menubar Menu bar object.
 * @return No return value.
 */
void ui_menubar_destroy(UiMenuBar* menubar);

/**
 * @brief Build and draw the menu bar.
 * @param menubar Menu bar object.
 * @param workspace Workspace object.
 * @param window_width Current window width.
 * @return No return value.
 */
void ui_menubar_build(UiMenuBar* menubar, struct Workspace* workspace, int window_width);

/**
 * @brief Query whether the inspector panel is visible.
 * @param menubar Menu bar object.
 * @return `true` if visible, `false` otherwise.
 */
bool ui_menubar_inspector_visible(const UiMenuBar* menubar);

/**
 * @brief Get the menu bar height.
 * @param menubar Menu bar object.
 * @return Menu bar height in pixels.
 */
float ui_menubar_height(const UiMenuBar* menubar);

/**
 * @brief Set the menu bar height.
 * @param menubar Menu bar object.
 * @param height New height in pixels.
 * @return No return value.
 */
void ui_menubar_set_height(UiMenuBar* menubar, float height);

/**
 * @brief Set the available theme list for the menu bar.
 * @param menubar Menu bar object.
 * @param themes Theme descriptor array.
 * @param theme_count Array length.
 * @return No return value.
 */
void ui_menubar_set_themes(UiMenuBar* menubar, const UiThemeDescriptor* themes, int theme_count);

/**
 * @brief Set the currently active theme index.
 * @param menubar Menu bar object.
 * @param theme_index Target theme index.
 * @return No return value.
 */
void ui_menubar_set_active_theme_index(UiMenuBar* menubar, int theme_index);

/**
 * @brief Take and clear the pending theme application request.
 * @param menubar Menu bar object.
 * @return Requested theme index; returns `-1` if no request is pending.
 */
int ui_menubar_take_theme_request(UiMenuBar* menubar);

/**
 * @brief Take and clear the theme reload request flag.
 * @param menubar Menu bar object.
 * @return `1` if a request is pending, `0` otherwise.
 */
int ui_menubar_take_theme_reload_request(UiMenuBar* menubar);

#endif /* GLDRAW_UI_UI_MENUBAR_H */
