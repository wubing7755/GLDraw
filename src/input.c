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
 * Why no Tool abstraction yet:
 *   Phase 1 has only one interaction mode (draw LINE).
 *   Tool abstraction will be introduced in Phase 3 when we add
 *   selection and multiple tools.
 * =============================================================================
 */

static GLFWwindow* s_window = NULL;
static int s_mouse_down = 0;
static float s_p1[2] = {0.0f, 0.0f};
static float s_p2[2] = {0.0f, 0.0f};
static int s_has_preview = 0;

/* Forward-declare Phase 1 preview — a temporary static line */
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
    if (!s_has_preview) {
        return;
    }

    /* s_preview_shape is already in ShapeManager — replace with opaque version */
    /* Remove the semi-transparent preview */
    sm_remove_last();  /* removes last added (the preview) */
    s_preview_shape = NULL;
    s_has_preview = 0;

    /* Add the final opaque line */
    Shape* final = shape_create_line(
        s_p1[0], s_p1[1], s_p2[0], s_p2[1],
        0.0f, 0.8f, 1.0f, 1.0f,  /* cyan, opaque */
        2.0f);
    sm_add(final);
    printf("[Input] Line finalized: (%.2f,%.2f) → (%.2f,%.2f)\n",
           s_p1[0], s_p1[1], s_p2[0], s_p2[1]);
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
        window_to_opengl(win_x, win_y, &s_p1[0], &s_p1[1]);
        s_p2[0] = s_p1[0];
        s_p2[1] = s_p1[1];
        s_has_preview = 0;
        s_mouse_down = 1;
        printf("[Input] Line start: (%.2f, %.2f)\n", s_p1[0], s_p1[1]);

    } else if (action == GLFW_RELEASE) {
        if (s_mouse_down) {
            finalize_line();
            s_mouse_down = 0;
        }
    }
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;
    if (!s_mouse_down) {
        return;
    }

    float x, y;
    window_to_opengl(xpos, ypos, &x, &y);

    if (s_has_preview) {
        /* Update existing preview — remove old preview, add new one */
        sm_remove_last();  /* remove old preview from manager */
        shape_destroy(s_preview_shape);
        s_preview_shape = NULL;
        s_has_preview = 0;
    }

    s_p2[0] = x;
    s_p2[1] = y;
    s_preview_shape = shape_create_line(s_p1[0], s_p1[1], s_p2[0], s_p2[1],
                                        1.0f, 1.0f, 1.0f, 0.7f, 2.0f);
    sm_add(s_preview_shape);
    s_has_preview = 1;
}

static void key_callback(GLFWwindow* window, int key, int scancode,
                         int action, int mods)
{
    (void)scancode;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        printf("[Input] ESC pressed\n");
    }

    /* Ctrl+Z — delete last line */
    if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_Z && action == GLFW_PRESS) {
        printf("[Input] Ctrl+Z — removing last line\n");
        sm_remove_last();
    }
}

void init_input(GLFWwindow* window)
{
    s_window = window;
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);
    printf("[Input] Initialized (mouse line drawing)\n");
}

void process_input(void)
{
    /* No held-key processing needed for Phase 1 */
}
