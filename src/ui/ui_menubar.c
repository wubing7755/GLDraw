#include "ui_menubar.h"

#include "ui_menu_def.h"
#include "ui_menu_actions.h"

#include <app/workspace.h>

#include <glad/glad.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include "nuklear/nuklear.h"
#include "nuklear/nuklear_glfw_gl3.h"
#include <ui/ui_theme.h>

#include <stdio.h>
#include <stdlib.h>

#define MENU_BAR_HEIGHT 30.0f
#define MENU_PARENT_THEME 900
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct UiTopMenuDef {
    const char* label;
    int parent_id;
    struct nk_vec2 popup_size;
    float item_width;
} UiTopMenuDef;

static const UiTopMenuDef g_top_menus[] = {
    { "File", MENU_ID_FILE, {210.0f, 300.0f}, 58.0f },
    { "Edit", MENU_ID_EDIT, {210.0f, 300.0f}, 58.0f },
    { "View", MENU_ID_VIEW, {220.0f, 300.0f}, 62.0f },
    { "Theme", MENU_PARENT_THEME, {260.0f, 280.0f}, 76.0f },
    { "Help", MENU_ID_HELP, {220.0f, 220.0f}, 62.0f },
};

typedef struct UiQuickActionDef {
    const char* label;
    MenuId menu_id;
    float width;
    const char* tooltip;
} UiQuickActionDef;

static const UiQuickActionDef g_quick_actions[] = {
    { "New", MENU_ID_FILE_NEW, 48.0f, "Create new document (Ctrl+N)" },
    { "Open", MENU_ID_FILE_OPEN, 52.0f, "Open document (Ctrl+O)" },
    { "Save", MENU_ID_FILE_SAVE, 52.0f, "Save document (Ctrl+S)" },
    { "Undo", MENU_ID_EDIT_UNDO, 52.0f, "Undo (Ctrl+Z)" },
    { "Redo", MENU_ID_EDIT_REDO, 52.0f, "Redo (Ctrl+Y)" },
    { "+", MENU_ID_VIEW_ZOOM_IN, 30.0f, "Zoom in (Ctrl++)" },
    { "-", MENU_ID_VIEW_ZOOM_OUT, 30.0f, "Zoom out (Ctrl+-)" },
    { "Fit", MENU_ID_VIEW_ZOOM_FIT, 40.0f, "Zoom to fit (Ctrl+0)" },
};

static void ui_build_menu_item_text(char* out_text, size_t out_size, const MenuItemDef* item)
{
    if (!out_text || out_size == 0 || !item || !item->label) {
        return;
    }

    if (item->shortcut && item->shortcut[0] != '\0') {
        snprintf(out_text, out_size, "%-20s %s", item->label, item->shortcut);
    } else {
        snprintf(out_text, out_size, "%s", item->label);
    }
}

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

void ui_menubar_destroy(UiMenuBar* menubar)
{
    free(menubar);
}

bool ui_menubar_inspector_visible(const UiMenuBar* menubar)
{
    return menubar ? menubar->show_inspector : true;
}

float ui_menubar_height(const UiMenuBar* menubar)
{
    return menubar ? menubar->menu_height : MENU_BAR_HEIGHT;
}

void ui_menubar_set_height(UiMenuBar* menubar, float height)
{
    if (!menubar) {
        return;
    }

    menubar->menu_height = (height > 10.0f) ? height : MENU_BAR_HEIGHT;
}

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
 * @brief Render dropdown menu items
 */
static int ui_render_dropdown(struct nk_context* ctx, int parent_id)
{
    int clicked_id = -1;
    const MenuItemDef* items = ui_menu_def_items();
    int count = ui_menu_def_count();

    for (int i = 0; i < count; i++) {
        const MenuItemDef* item = &items[i];

        if (item->parent_id != parent_id) {
            continue;
        }

        switch (item->type) {
        case MENU_ITEM_SEPARATOR:
            nk_labelf(ctx, NK_TEXT_CENTERED, "----------------");
            break;

        case MENU_ITEM_SUBMENU:
            if (item->label && item->label[0] != '\0') {
                nk_widget_disable_begin(ctx);
                nk_menu_item_label(ctx, item->label, NK_TEXT_LEFT);
                nk_widget_disable_end(ctx);
            }
            break;

        case MENU_ITEM_ACTION: {
            char menu_text[96];
            int enabled = ui_menu_is_action_available((MenuId)item->id);

            ui_build_menu_item_text(menu_text, sizeof(menu_text), item);

            if (!enabled) {
                nk_widget_disable_begin(ctx);
            }
            if (nk_menu_item_label(ctx, menu_text, NK_TEXT_LEFT) && enabled) {
                clicked_id = item->id;
            }
            if (!enabled) {
                nk_widget_disable_end(ctx);
            }
            break;
        }
        }
    }

    return clicked_id;
}

static int ui_render_theme_dropdown(UiMenuBar* menubar)
{
    struct nk_context* ctx = NULL;

    if (!menubar) {
        return -1;
    }

    ctx = menubar->ctx;
    if (!ctx) {
        return -1;
    }

    if (!menubar->themes || menubar->theme_count <= 0) {
        nk_label(ctx, "No themes available", NK_TEXT_LEFT);
        return -1;
    }

    if (nk_menu_item_label(ctx, "Reload Themes", NK_TEXT_LEFT)) {
        menubar->requested_theme_reload = 1;
    }
    nk_labelf(ctx, NK_TEXT_CENTERED, "----------------");

    for (int i = 0; i < menubar->theme_count; ++i) {
        const UiThemeDescriptor* theme = &menubar->themes[i];
        char item_label[128];
        const char* label = (theme->label && theme->label[0] != '\0') ? theme->label : "Unnamed Theme";
        const char* marker = (i == menubar->active_theme_index) ? "[x]" : "[ ]";

        snprintf(item_label, sizeof(item_label), "%s %s", marker, label);
        if (nk_menu_item_label(ctx, item_label, NK_TEXT_LEFT)) {
            return i;
        }
    }

    return -1;
}

static int ui_render_top_menu(UiMenuBar* menubar, const UiTopMenuDef* menu)
{
    struct nk_context* ctx = NULL;
    int clicked_id = -1;

    if (!menubar || !menu || !menu->label) {
        return -1;
    }
    ctx = menubar->ctx;
    if (!ctx) {
        return -1;
    }

    if (nk_menu_begin_label(ctx, menu->label, NK_TEXT_LEFT, menu->popup_size)) {
        nk_layout_row_dynamic(ctx, 25.0f, 1);
        if (menu->parent_id == MENU_PARENT_THEME) {
            int theme_index = ui_render_theme_dropdown(menubar);
            if (theme_index >= 0) {
                menubar->requested_theme_index = theme_index;
            }
        } else {
            clicked_id = ui_render_dropdown(ctx, menu->parent_id);
        }
        nk_menu_end(ctx);
    }

    return clicked_id;
}

static void ui_dispatch_menu_action(UiMenuBar* menubar, Workspace* workspace, int clicked_id)
{
    if (!menubar || !workspace || clicked_id == -1) {
        return;
    }

    if (clicked_id == MENU_ID_VIEW_TOGGLE_INSPECTOR) {
        menubar->show_inspector = !menubar->show_inspector;
        return;
    }

    ui_menu_execute(workspace, (MenuId)clicked_id);
}

void ui_menubar_build(UiMenuBar* menubar, Workspace* workspace, int window_width)
{
    struct nk_context* ctx;
    int clicked_id = -1;
    size_t menu_count = ARRAY_SIZE(g_top_menus);
    size_t quick_count = ARRAY_SIZE(g_quick_actions);
    float left_width_total = 0.0f;
    float right_width_total = 0.0f;
    float spacer_width = 8.0f;
    float bar_height;
    float row_height;
    int columns;

    if (!menubar || !workspace) {
        return;
    }

    ctx = menubar->ctx;
    if (!ctx) {
        return;
    }

    for (size_t i = 0; i < menu_count; i++) {
        left_width_total += g_top_menus[i].item_width;
    }
    for (size_t i = 0; i < quick_count; i++) {
        right_width_total += g_quick_actions[i].width;
    }

    bar_height = ui_menubar_height(menubar);
    row_height = bar_height - 6.0f;
    if (row_height < 20.0f) {
        row_height = bar_height;
    }

    spacer_width = (float)window_width - left_width_total - right_width_total - 18.0f;
    if (spacer_width < 8.0f) {
        spacer_width = 8.0f;
    }
    columns = (int)(menu_count + quick_count + 1u);

    /* Menu bar window - nk_menubar_begin must be first function after nk_begin */
    if (nk_begin(ctx, "##menubar##",
                 nk_rect(0.0f, 0.0f, (float)window_width, bar_height),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

        /* nk_menubar_begin must be called immediately after nk_begin */
        nk_menubar_begin(ctx);

        nk_layout_row_begin(ctx, NK_STATIC, row_height, columns);
        for (size_t i = 0; i < menu_count; i++) {
            int id;
            nk_layout_row_push(ctx, g_top_menus[i].item_width);
            id = ui_render_top_menu(menubar, &g_top_menus[i]);
            if (id != -1) {
                clicked_id = id;
            }
        }

        nk_layout_row_push(ctx, spacer_width);
        nk_label(ctx, "", NK_TEXT_LEFT);

        for (size_t i = 0; i < quick_count; i++) {
            struct nk_rect widget_bounds;
            int enabled = ui_menu_is_action_available(g_quick_actions[i].menu_id);
            int hovered = 0;
            nk_layout_row_push(ctx, g_quick_actions[i].width);
            if (!enabled) {
                nk_widget_disable_begin(ctx);
            }
            widget_bounds = nk_widget_bounds(ctx);
            if (nk_button_label(ctx, g_quick_actions[i].label)) {
                clicked_id = g_quick_actions[i].menu_id;
            }
            hovered = nk_input_is_mouse_hovering_rect(&ctx->input, widget_bounds);
            if (g_quick_actions[i].tooltip &&
                g_quick_actions[i].tooltip[0] != '\0' &&
                hovered) {
                nk_tooltip(ctx, g_quick_actions[i].tooltip);
            }
            if (!enabled) {
                nk_widget_disable_end(ctx);
            }
        }

        nk_layout_row_end(ctx);
        nk_menubar_end(ctx);
    }
    nk_end(ctx);

    /* Execute clicked action */
    ui_dispatch_menu_action(menubar, workspace, clicked_id);
}
