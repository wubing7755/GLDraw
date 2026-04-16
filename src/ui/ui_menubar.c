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

#include <stdlib.h>

#define MENU_BAR_HEIGHT 30.0f
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct UiTopMenuDef {
    const char* label;
    int parent_id;
    struct nk_vec2 popup_size;
} UiTopMenuDef;

static const UiTopMenuDef g_top_menus[] = {
    { "File", MENU_ID_FILE, {120.0f, 300.0f} },
    { "Edit", MENU_ID_EDIT, {120.0f, 300.0f} },
    { "View", MENU_ID_VIEW, {150.0f, 300.0f} },
    { "Help", MENU_ID_HELP, {150.0f, 200.0f} },
};

UiMenuBar* ui_menubar_create(void* ctx)
{
    UiMenuBar* menubar = (UiMenuBar*)calloc(1, sizeof(*menubar));
    if (!menubar) {
        return NULL;
    }

    menubar->ctx = (struct nk_context*)ctx;
    menubar->show_inspector = true;
    menubar->menu_height = MENU_BAR_HEIGHT;

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
            break;

        case MENU_ITEM_ACTION:
            if (nk_menu_item_label(ctx, item->label, NK_TEXT_LEFT)) {
                clicked_id = item->id;
            }
            break;
        }
    }

    return clicked_id;
}

static int ui_render_top_menu(struct nk_context* ctx, const UiTopMenuDef* menu)
{
    int clicked_id = -1;

    if (!ctx || !menu || !menu->label) {
        return -1;
    }

    if (nk_menu_begin_label(ctx, menu->label, NK_TEXT_LEFT, menu->popup_size)) {
        nk_layout_row_dynamic(ctx, 25.0f, 1);
        clicked_id = ui_render_dropdown(ctx, menu->parent_id);
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

    if (!menubar || !workspace) {
        return;
    }

    ctx = menubar->ctx;
    if (!ctx) {
        return;
    }

    /* Menu bar window - nk_menubar_begin must be first function after nk_begin */
    if (nk_begin(ctx, "##menubar##",
                  nk_rect(0.0f, 0.0f, (float)window_width, MENU_BAR_HEIGHT),
                  NK_WINDOW_NO_SCROLLBAR)) {

        /* nk_menubar_begin must be called immediately after nk_begin */
        nk_menubar_begin(ctx);

        /* Render top-level menus using data-driven definitions */
        nk_layout_row_begin(ctx, NK_DYNAMIC, MENU_BAR_HEIGHT, (int)menu_count);
        for (size_t i = 0; i < menu_count; i++) {
            int id;
            nk_layout_row_push(ctx, 1.0f / (float)menu_count);
            id = ui_render_top_menu(ctx, &g_top_menus[i]);
            if (id != -1) {
                clicked_id = id;
            }
        }

        nk_layout_row_end(ctx);
        nk_menubar_end(ctx);
    }
    nk_end(ctx);

    /* Execute clicked action */
    ui_dispatch_menu_action(menubar, workspace, clicked_id);
}
