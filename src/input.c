#include <stdio.h>
#include <GLFW/glfw3.h>
#include <core/window.h>
#include <core/tool_manager.h>
#include <core/selection_manager.h>
#include <core/shape_manager.h>
#include <core/draw_tool.h>
#include <core/select_tool.h>

/* =============================================================================
 * Phase 3: Input handling — delegates to ToolManager
 *
 * All mouse/key events are forwarded to the current tool's vtable.
 * =============================================================================
 */

static GLFWwindow* s_window = NULL;
static SelectionManager s_selection;
static Tool* s_draw_tool = NULL;
static Tool* s_select_tool = NULL;
static Tool* s_current_draw_tool = NULL;  /* the draw tool (LINE/CIRCLE/RECT) */

/* Convert window coords to OpenGL normalized coords */
static void window_to_opengl(double win_x, double win_y, float* out_x, float* out_y)
{
    int width, height;
    glfwGetFramebufferSize(s_window, &width, &height);

    /* Window coords: (0,0) top-left → OpenGL coords: (-1,-1) bottom-left */
    *out_x = (float)(win_x / (double)width * 2.0 - 1.0);
    *out_y = (float)(1.0 - win_y / (double)height * 2.0);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;

    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }

    double win_x, win_y;
    glfwGetCursorPos(window, &win_x, &win_y);

    float x, y;
    window_to_opengl(win_x, win_y, &x, &y);

    Tool* tool = toolmanager_get_current();
    if (!tool || !tool->vtable || !tool->vtable->on_down) {
        return;
    }

    if (action == GLFW_PRESS) {
        tool->vtable->on_down(tool, x, y, &s_selection);
    } else if (action == GLFW_RELEASE) {
        tool->vtable->on_up(tool, &s_selection);
    }
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;

    float x, y;
    window_to_opengl(xpos, ypos, &x, &y);

    Tool* tool = toolmanager_get_current();
    if (!tool || !tool->vtable || !tool->vtable->on_move) {
        return;
    }

    tool->vtable->on_move(tool, x, y, &s_selection);
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
        if (key == GLFW_KEY_1 && s_current_draw_tool) {
            /* LINE */
            draw_tool_set_type(s_current_draw_tool, "LINE");
            toolmanager_set_tool(s_current_draw_tool);
        } else if (key == GLFW_KEY_2 && s_current_draw_tool) {
            /* CIRCLE */
            draw_tool_set_type(s_current_draw_tool, "CIRCLE");
            toolmanager_set_tool(s_current_draw_tool);
        } else if (key == GLFW_KEY_3 && s_current_draw_tool) {
            /* RECT */
            draw_tool_set_type(s_current_draw_tool, "RECT");
            toolmanager_set_tool(s_current_draw_tool);
        } else if (key == GLFW_KEY_S && s_select_tool) {
            /* SELECT tool */
            toolmanager_set_tool(s_select_tool);
        }
    }

    /* Ctrl+Z — delete last shape */
    if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_Z && action == GLFW_PRESS) {
        sm_remove_last();
    }
}

SelectionManager* input_get_selection(void)
{
    return &s_selection;
}

void init_input(GLFWwindow* window)
{
    s_window = window;
    sel_init(&s_selection);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);
}

void input_init_tools(Tool* draw_tool, Tool* select_tool, Tool* default_tool)
{
    s_draw_tool = draw_tool;
    s_select_tool = select_tool;
    s_current_draw_tool = draw_tool;
    toolmanager_set_tool(default_tool);
}

void process_input(void)
{
    /* No held-key processing needed */
}
