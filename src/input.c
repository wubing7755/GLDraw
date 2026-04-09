#include <GLFW/glfw3.h>
#include <core/window.h>
#include <core/app_state.h>
#include <core/tool_manager.h>
#include <core/shape_manager.h>
#include <core/draw_tool.h>
#include <core/select_tool.h>
#include <core/nuklear_ui.h>
#include <core/macros.h>

/* Base resolution for coordinate system - must match renderer.c */
#define BASE_RESOLUTION_WIDTH  800
#define BASE_RESOLUTION_HEIGHT 600

/* =============================================================================
 * Phase 3: Input handling — delegates to ToolManager
 *
 * All mouse/key events are forwarded to the current tool's vtable.
 * =============================================================================
 */

/* Convert window coords to canvas coords (top-left origin, Y downward).
 * Window: (0,0) at top-left, Y increases downward.
 * Canvas: (0,0) at top-left, Y increases downward.  <-- 与窗口一致！
 * Graphics: rendered with Y-up in OpenGL, but canvas coords use Y-down.
 */
static void window_to_opengl(double win_x, double win_y, float* out_x, float* out_y)
{
    int win_width, win_height;
    glfwGetWindowSize(g_app_state.window, &win_width, &win_height);

    if (win_width <= 0 || win_height <= 0) {
        *out_x = 0.0f;
        *out_y = 0.0f;
        return;
    }

    /* Same scaling and offset as renderer projection */
    float scale_x = (float)win_width / BASE_RESOLUTION_WIDTH;
    float scale_y = (float)win_height / BASE_RESOLUTION_HEIGHT;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    float scaled_width = BASE_RESOLUTION_WIDTH * scale;
    float scaled_height = BASE_RESOLUTION_HEIGHT * scale;
    float offset_x = (win_width - scaled_width) * 0.5f;
    float offset_y = (win_height - scaled_height) * 0.5f;

    /* Check if click is within canvas area */
    if (win_x < offset_x || win_x >= offset_x + scaled_width ||
        win_y < offset_y || win_y >= offset_y + scaled_height) {
        *out_x = 0.0f;
        *out_y = 0.0f;
        return;
    }

    /* Map window coords to canvas coords (Y flipped: window Y-down, canvas Y-down) */
    *out_x = (float)((win_x - offset_x) / scale);
    *out_y = (float)(((offset_y + scaled_height) - win_y) / scale);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }

    double win_x, win_y;
    glfwGetCursorPos(window, &win_x, &win_y);

    if (action == GLFW_PRESS && nuklear_ui_blocks_mouse_input(win_x, win_y)) {
        g_app_state.tool_mouse_active = 0;
        return;
    }

    float x, y;
    window_to_opengl(win_x, win_y, &x, &y);

    Tool* tool = toolmanager_get_current();
    if (!tool || !tool->vtable || !tool->vtable->on_down) {
        return;
    }

    if (action == GLFW_PRESS) {
        int shift_held = (mods & GLFW_MOD_SHIFT) != 0;
        tool->vtable->on_down(tool, x, y, &g_app_state.selection, shift_held);
        g_app_state.tool_mouse_active = 1;
    } else if (action == GLFW_RELEASE && g_app_state.tool_mouse_active) {
        tool->vtable->on_up(tool, &g_app_state.selection);
        g_app_state.tool_mouse_active = 0;
    }
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;

    if (!g_app_state.tool_mouse_active && nuklear_ui_blocks_mouse_input(xpos, ypos)) {
        return;
    }

    float x, y;
    window_to_opengl(xpos, ypos, &x, &y);

    Tool* tool = toolmanager_get_current();
    if (!tool || !tool->vtable || !tool->vtable->on_move) {
        return;
    }

    tool->vtable->on_move(tool, x, y, &g_app_state.selection);
}

static void key_callback(GLFWwindow* window, int key, int scancode,
                         int action, int mods)
{
    (void)scancode;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    /* Tool shortcuts */
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1 && g_app_state.current_draw_tool) {
            /* LINE */
            draw_tool_set_type(g_app_state.current_draw_tool, "LINE");
            toolmanager_set_tool(g_app_state.current_draw_tool);
            LOG_DEBUG("Tool switched: LINE");
        } else if (key == GLFW_KEY_2 && g_app_state.current_draw_tool) {
            /* CIRCLE */
            draw_tool_set_type(g_app_state.current_draw_tool, "CIRCLE");
            toolmanager_set_tool(g_app_state.current_draw_tool);
            LOG_DEBUG("Tool switched: CIRCLE");
        } else if (key == GLFW_KEY_3 && g_app_state.current_draw_tool) {
            /* RECT */
            draw_tool_set_type(g_app_state.current_draw_tool, "RECT");
            toolmanager_set_tool(g_app_state.current_draw_tool);
            LOG_DEBUG("Tool switched: RECT");
        } else if (key == GLFW_KEY_S && g_app_state.select_tool) {
            /* SELECT tool */
            toolmanager_set_tool(g_app_state.select_tool);
            LOG_DEBUG("Tool switched: SELECT");
        }
    }

    /* Ctrl+Z — delete last shape */
    if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_Z && action == GLFW_PRESS) {
        Shape* removed = sm_take_last();
        if (removed) {
            sel_remove(&g_app_state.selection, removed);
            shape_destroy(removed);
        }
    }
}

void init_input(GLFWwindow* window)
{
    app_state_init(window);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);
}

void input_init_tools(Tool* draw_tool, Tool* select_tool, Tool* default_tool)
{
    app_state_set_tools(draw_tool, select_tool, default_tool);
    toolmanager_set_tool(default_tool);
}

void process_input(void)
{
    /* No held-key processing needed */
}
