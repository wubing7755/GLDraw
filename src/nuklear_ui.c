#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <core/nuklear_ui.h>
#include <core/app_state.h>
#include <core/shape.h>
#include <core/macros.h>

/* Nuklear configuration macros - must be defined before including Nuklear */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION

#include "nuklear/nuklear.h"
#include "nuklear/nuklear_glfw_gl3.h"

static struct nk_glfw g_glfw_ctx;
static struct nk_context* s_ctx;
static struct nk_rect s_property_panel_bounds;

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

int init_nuklear_ui(GLFWwindow* window)
{
    s_ctx = nk_glfw3_init(&g_glfw_ctx, window, 0 /* NK_GLFW3_INSTALL_CALLBACKS */);
    if (UNLIKELY(!s_ctx)) {
        LOG_ERROR("Failed to initialize Nuklear");
        return -1;
    }
    LOG_DEBUG("Nuklear initialized (callbacks disabled for Phase 1)");

    struct nk_font_atlas* atlas;
    nk_glfw3_font_stash_begin(&g_glfw_ctx, &atlas);
    nk_glfw3_font_stash_end(&g_glfw_ctx);

    s_property_panel_bounds = nk_rect(580.0f, 50.0f, 220.0f, 450.0f);

    return 0;
}

void nuklear_new_frame(void)
{
    nk_glfw3_new_frame(&g_glfw_ctx);
}

void nuklear_build_ui(void)
{
    /* Property Panel — Color & line width editing with multi-selection support */
    if (nk_begin(s_ctx, "Property Panel", s_property_panel_bounds,
                 NK_WINDOW_BORDER)) {
        s_property_panel_bounds = nk_window_get_bounds(s_ctx);
        SelectionManager* sel = app_state_get_selection();
        if (sel && sel_count(sel) > 0) {
            Shape* first = sel_get(sel, 0);

            /* Type display (Step 4a - read-only) */
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, "Type:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, first->vtable->name, NK_TEXT_LEFT);

            /* Color section (Step 4b) */
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, "Color:", NK_TEXT_LEFT);

            float r = 0, g = 0, b = 0, a = 0;
            /* Read from first shape */
            shape_get_property(first, "color_r", &r);
            shape_get_property(first, "color_g", &g);
            shape_get_property(first, "color_b", &b);
            shape_get_property(first, "color_a", &a);

            nk_layout_row_dynamic(s_ctx, 25, 1);
            nk_label(s_ctx, "R:", NK_TEXT_LEFT);
            nk_slider_float(s_ctx, 0.0f, &r, 1.0f, 0.01f);
            for (int i = 0; i < sel_count(sel); i++) {
                shape_set_property(sel_get(sel, i), "color_r", r);
            }

            nk_layout_row_dynamic(s_ctx, 25, 1);
            nk_label(s_ctx, "G:", NK_TEXT_LEFT);
            nk_slider_float(s_ctx, 0.0f, &g, 1.0f, 0.01f);
            for (int i = 0; i < sel_count(sel); i++) {
                shape_set_property(sel_get(sel, i), "color_g", g);
            }

            nk_layout_row_dynamic(s_ctx, 25, 1);
            nk_label(s_ctx, "B:", NK_TEXT_LEFT);
            nk_slider_float(s_ctx, 0.0f, &b, 1.0f, 0.01f);
            for (int i = 0; i < sel_count(sel); i++) {
                shape_set_property(sel_get(sel, i), "color_b", b);
            }

            nk_layout_row_dynamic(s_ctx, 25, 1);
            nk_label(s_ctx, "A:", NK_TEXT_LEFT);
            nk_slider_float(s_ctx, 0.0f, &a, 1.0f, 0.01f);
            for (int i = 0; i < sel_count(sel); i++) {
                shape_set_property(sel_get(sel, i), "color_a", a);
            }

            /* Line width section (Step 4c) */
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, "Line Width:", NK_TEXT_LEFT);

            float lw = 1.0f;
            shape_get_property(first, "line_width", &lw);
            nk_slider_float(s_ctx, 1.0f, &lw, 10.0f, 0.1f);
            for (int i = 0; i < sel_count(sel); i++) {
                shape_set_property(sel_get(sel, i), "line_width", lw);
            }

            /* Selection count (Step 4d) */
            nk_layout_row_dynamic(s_ctx, 20, 1);
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "Selected: %d", sel_count(sel));
                nk_label(s_ctx, buf, NK_TEXT_LEFT);
            }
        } else {
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, "No selection", NK_TEXT_LEFT);
        }
    }
    nk_end(s_ctx);
}

int nuklear_ui_blocks_mouse_input(double x, double y)
{
    if (!s_ctx) {
        return 0;
    }

    if (nk_item_is_any_active(s_ctx)) {
        return 1;
    }

    return x >= s_property_panel_bounds.x &&
           x <= s_property_panel_bounds.x + s_property_panel_bounds.w &&
           y >= s_property_panel_bounds.y &&
           y <= s_property_panel_bounds.y + s_property_panel_bounds.h;
}

void nuklear_render(void)
{
    /* Nuklear renders after OpenGL frame — submit accumulated commands */
    nk_glfw3_render(&g_glfw_ctx, NK_ANTI_ALIASING_ON,
                    MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}

void shutdown_nuklear_ui(void)
{
    nk_glfw3_shutdown(&g_glfw_ctx);
    LOG_DEBUG("Nuklear shutdown complete");
}
