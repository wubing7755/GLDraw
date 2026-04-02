#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <core/nuklear_ui.h>
#include <core/app_state.h>

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
static struct nk_context* g_ctx;

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

int init_nuklear_ui(GLFWwindow* window)
{
    g_ctx = nk_glfw3_init(&g_glfw_ctx, window, NK_GLFW3_INSTALL_CALLBACKS);
    if (!g_ctx)
    {
        printf("[Nuklear] Failed to initialize\n");
        return -1;
    }
    printf("[Nuklear] Initialized\n");

    struct nk_font_atlas* atlas;
    nk_glfw3_font_stash_begin(&g_glfw_ctx, &atlas);
    nk_glfw3_font_stash_end(&g_glfw_ctx);

    return 0;
}

void nuklear_new_frame(void)
{
    nk_glfw3_new_frame(&g_glfw_ctx);
}

void nuklear_build_ui(void)
{
    if (nk_begin(g_ctx, "Debug Controls",
                 nk_rect(10, 10, 280, 220),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
                 NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        nk_layout_row_static(g_ctx, 25, 120, 2);
        nk_label(g_ctx, "Color Red:", NK_TEXT_LEFT);
        nk_slider_float(g_ctx, 0.0f, &g_app_state.color[0], 1.0f, 0.01f);

        nk_layout_row_static(g_ctx, 25, 120, 2);
        nk_label(g_ctx, "Color Green:", NK_TEXT_LEFT);
        nk_slider_float(g_ctx, 0.0f, &g_app_state.color[1], 1.0f, 0.01f);

        nk_layout_row_static(g_ctx, 25, 120, 2);
        nk_label(g_ctx, "Color Blue:", NK_TEXT_LEFT);
        nk_slider_float(g_ctx, 0.0f, &g_app_state.color[2], 1.0f, 0.01f);

        nk_layout_row_static(g_ctx, 30, 100, 1);
        if (nk_button_label(g_ctx, "Reset Color"))
        {
            g_app_state.color[0] = 1.0f;
            g_app_state.color[1] = 0.0f;
            g_app_state.color[2] = 0.0f;
            printf("[Nuklear] Color reset\n");
        }

        nk_layout_row_static(g_ctx, 20, 200, 1);
        char color_text[64];
        snprintf(color_text, sizeof(color_text),
                 "RGB: (%.2f, %.2f, %.2f)",
                 g_app_state.color[0], g_app_state.color[1], g_app_state.color[2]);
        nk_label(g_ctx, color_text, NK_TEXT_LEFT);
    }
    nk_end(g_ctx);
}

void nuklear_render(void)
{
    nk_glfw3_render(&g_glfw_ctx, NK_ANTI_ALIASING_ON,
                    MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}

void shutdown_nuklear_ui(void)
{
    nk_glfw3_shutdown(&g_glfw_ctx);
    printf("[Nuklear] Shutdown complete\n");
}
