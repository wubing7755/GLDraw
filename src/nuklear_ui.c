#include <stdio.h>
#include <string.h>
#include <math.h>
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

/* Returns 1 if all shapes have the same value for the property, 0 if mixed.
   Sets *out_value to the common value (or first shape's value if mixed). */
static int get_common_property_value(SelectionManager* sel, const char* key,
                                     float* out_value, int* out_is_common)
{
    float first_val = 0;
    shape_get_property(sel_get(sel, 0), key, &first_val);
    *out_value = first_val;
    *out_is_common = 1;

    for (int i = 1; i < sel_count(sel); i++) {
        float val = 0;
        shape_get_property(sel_get(sel, i), key, &val);
        if (fabsf(val - first_val) > 1e-6f) {
            *out_is_common = 0;
            return 0;
        }
    }
    return 1;
}

/* Helper to create a labeled slider that applies to all selected shapes.
   Shows "--" when selected shapes have mixed values. */
static void property_slider_row(SelectionManager* sel, const char* label,
                                const char* key, float min, float max, float step)
{
    float value = 0;
    int is_common = 0;
    get_common_property_value(sel, key, &value, &is_common);

    nk_layout_row_dynamic(s_ctx, 20, 2);
    nk_label(s_ctx, label, NK_TEXT_LEFT);

    if (!is_common) {
        /* Mixed values - show "--" and skip slider */
        nk_label(s_ctx, "--", NK_TEXT_LEFT);
    } else {
        int changed = nk_slider_float(s_ctx, min, &value, max, step);
        if (changed) {
            for (int i = 0; i < sel_count(sel); i++) {
                shape_set_property(sel_get(sel, i), key, value);
            }
        }
    }
}

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
    /* Calculate responsive panel bounds based on window size */
    int window_width, window_height;
    GLFWwindow* win = glfwGetCurrentContext();
    glfwGetWindowSize(win, &window_width, &window_height);

    int panel_width = 220;
    int panel_height = 450;
    int panel_x = window_width - panel_width - 10;
    int panel_y = 50;

    /* Clamp if window is too small */
    if (panel_x < 10) panel_x = 10;
    if (panel_y + panel_height > window_height - 10) {
        panel_height = window_height - panel_y - 10;
    }
    if (panel_height < 100) panel_height = 100;

    s_property_panel_bounds = nk_rect((float)panel_x, (float)panel_y,
                                      (float)panel_width, (float)panel_height);

    /* Property Panel — displays and edits properties of selected shapes */
    if (nk_begin(s_ctx, "Property Panel", s_property_panel_bounds,
                 NK_WINDOW_BORDER)) {
        s_property_panel_bounds = nk_window_get_bounds(s_ctx);
        SelectionManager* sel = app_state_get_selection();
        if (sel && sel_count(sel) > 0) {
            Shape* first = sel_get(sel, 0);
            const char* type = first->vtable->name;

            /* Type display (read-only) */
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, "Type:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, type, NK_TEXT_LEFT);

            /* Shape-specific position/size editing */
            if (strcmp(type, "LINE") == 0) {
                nk_layout_row_dynamic(s_ctx, 20, 1);
                nk_label(s_ctx, "Start:", NK_TEXT_LEFT);
                property_slider_row(sel, "X:", "p1_x", -1.0f, 1.0f, 0.01f);
                property_slider_row(sel, "Y:", "p1_y", -1.0f, 1.0f, 0.01f);

                nk_layout_row_dynamic(s_ctx, 20, 1);
                nk_label(s_ctx, "End:", NK_TEXT_LEFT);
                property_slider_row(sel, "X:", "p2_x", -1.0f, 1.0f, 0.01f);
                property_slider_row(sel, "Y:", "p2_y", -1.0f, 1.0f, 0.01f);
            } else if (strcmp(type, "CIRCLE") == 0) {
                nk_layout_row_dynamic(s_ctx, 20, 1);
                nk_label(s_ctx, "Center:", NK_TEXT_LEFT);
                property_slider_row(sel, "X:", "center_x", -1.0f, 1.0f, 0.01f);
                property_slider_row(sel, "Y:", "center_y", -1.0f, 1.0f, 0.01f);

                property_slider_row(sel, "Radius:", "radius", 0.01f, 0.5f, 0.01f);
            } else if (strcmp(type, "RECT") == 0) {
                property_slider_row(sel, "X:", "x", -1.0f, 1.0f, 0.01f);
                property_slider_row(sel, "Y:", "y", -1.0f, 1.0f, 0.01f);
                property_slider_row(sel, "Width:", "width", 0.01f, 1.0f, 0.01f);
                property_slider_row(sel, "Height:", "height", 0.01f, 1.0f, 0.01f);
            }

            /* Color editing (shared by all shape types) */
            nk_layout_row_dynamic(s_ctx, 20, 1);
            nk_label(s_ctx, "Color:", NK_TEXT_LEFT);
            property_slider_row(sel, "R:", "color_r", 0.0f, 1.0f, 0.01f);
            property_slider_row(sel, "G:", "color_g", 0.0f, 1.0f, 0.01f);
            property_slider_row(sel, "B:", "color_b", 0.0f, 1.0f, 0.01f);
            property_slider_row(sel, "A:", "color_a", 0.0f, 1.0f, 0.01f);

            /* Line width editing (shared by all shape types) */
            property_slider_row(sel, "Line Width:", "line_width", 1.0f, 10.0f, 0.1f);

            /* Selection count */
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
