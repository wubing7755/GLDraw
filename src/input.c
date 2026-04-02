#include <stdio.h>
#include <GLFW/glfw3.h>
#include <core/input.h>
#include <core/app_state.h>

static GLFWwindow* s_window = NULL;

static void key_callback(GLFWwindow* window, int key, int scancode,
                         int action, int mods)
{
    (void)scancode;
    (void)mods;

    /* ESC - close window */
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        printf("[Input] ESC pressed - window will close\n");
    }

    /* W - move up */
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        g_app_state.offset_y += 0.1f;
        printf("[Input] W pressed - triangle Y: %.2f\n", g_app_state.offset_y);
    }

    /* S - move down */
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        g_app_state.offset_y -= 0.1f;
        printf("[Input] S pressed - triangle Y: %.2f\n", g_app_state.offset_y);
    }
}

void init_input(GLFWwindow* window)
{
    s_window = window;
    glfwSetKeyCallback(window, key_callback);
    printf("[Input] Input system initialized\n");
}

void process_input(void)
{
    /* Handle held-down keys here if needed */
}
