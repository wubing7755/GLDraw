#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <core/window.h>
#include "shape.h"
#include "shape_manager.h"

/* =============================================================================
 * Phase 1: Mouse-based line drawing — no vtable, no tool abstraction
 *
 * Interaction:
 *   Left mouse button down  → start drawing (record p1)
 *   Mouse move (while down) → preview line (update p2)
 *   Left mouse button up    → finalize line, add to ShapeManager
 *   Ctrl+Z                  → delete last line
 *
 * Design:
 *   s_preview_shape is kept SEPARATE from ShapeManager until finalized.
 *   This avoids the double-free bug where preview existed in both places.
 * =============================================================================
 */

static GLFWwindow* s_window = NULL;
static int s_mouse_down = 0;
static float s_p1[2] = {0.0f, 0.0f};
static float s_p2[2] = {0.0f, 0.0f};
static int s_has_preview = 0;
static Shape* s_preview_shape = NULL;

/* Convert window coords to OpenGL normalized coords */
static void window_to_opengl(double win_x, double win_y, float* out_x, float* out_y)
{
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);

    /* Window coords: (0,0) top-left → OpenGL coords: (-1,-1) bottom-left */
    *out_x = (float)(win_x / (double)width * 2.0 - 1.0);
    *out_y = (float)(1.0 - win_y / (double)height * 2.0);
}

static void finalize_line(void)
{
    if (!s_has_preview || !s_preview_shape) {
        return;
    }

    /* Preview was added to ShapeManager — sm_remove_last() destroys it */
    sm_remove_last();  /* removes and destroys the preview from ShapeManager */
    s_preview_shape = NULL;  /* NULL it — already destroyed by sm_remove_last */
    s_has_preview = 0;

    /* Add the final opaque line to ShapeManager */
    Shape* final = shape_create_line(
        s_p1[0], s_p1[1], s_p2[0], s_p2[1],
        0.0f, 0.8f, 1.0f, 1.0f,  /* cyan, opaque */
        2.0f);
    sm_add(final);
}

static void update_preview(float x, float y)
{
    /* Destroy previous temp preview if exists */
    if (s_preview_shape) {
        shape_destroy(s_preview_shape);
        s_preview_shape = NULL;
    }

    s_p2[0] = x;
    s_p2[1] = y;

    /* Create temp preview and add to ShapeManager for rendering */
    s_preview_shape = shape_create_line(
        s_p1[0], s_p1[1], s_p2[0], s_p2[1],
        1.0f, 1.0f, 1.0f, 0.7f,  /* white, semi-transparent */
        2.0f);
    sm_add(s_preview_shape);
}

static void destroy_preview(void)
{
    if (s_preview_shape) {
        shape_destroy(s_preview_shape);
        s_preview_shape = NULL;
        s_has_preview = 0;
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;

    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }

    double win_x, win_y;
    glfwGetCursorPos(window, &win_x, &win_y);

    if (action == GLFW_PRESS) {
        /* Start fresh — destroy any leftover preview */
        destroy_preview();

        window_to_opengl(win_x, win_y, &s_p1[0], &s_p1[1]);
        s_p2[0] = s_p1[0];
        s_p2[1] = s_p1[1];
        s_mouse_down = 1;

    } else if (action == GLFW_RELEASE) {
        if (s_mouse_down) {
            update_preview((float)s_p2[0], (float)s_p2[1]);
            finalize_line();
            s_mouse_down = 0;
        }
    }
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;
    if (!s_mouse_down) {
        /* If mouse is released but we still have a preview, finalize it
         * (handles edge case where release event fires without move) */
        if (s_has_preview) {
            finalize_line();
        }
        return;
    }

    float x, y;
    window_to_opengl(xpos, ypos, &x, &y);
    update_preview(x, y);
}

static void key_callback(GLFWwindow* window, int key, int scancode,
                         int action, int mods)
{
    (void)scancode;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    /* Ctrl+Z — delete last line */
    if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_Z && action == GLFW_PRESS) {
        destroy_preview();  /* also destroy temp preview if active */
        sm_remove_last();
    }
}

void init_input(GLFWwindow* window)
{
    s_window = window;
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);
}

void process_input(void)
{
    /* No held-key processing needed for Phase 1 */
}
