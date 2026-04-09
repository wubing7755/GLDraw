#include <stdio.h>
#include <GLFW/glfw3.h>
#include <core/window.h>
#include <core/renderer.h>
#include <core/macros.h>

static GLFWwindow* s_window = NULL;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    (void)width;
    (void)height;
    /* renderer_on_viewport_change() calls glfwGetWindowSize internally
     * and update_projection() sets the correct letterbox viewport */
    renderer_on_viewport_change();
    LOG_DEBUG_F("Window resized: %dx%d", width, height);
}

int init_window(void)
{
    /* Initialize GLFW */
    if (UNLIKELY(!glfwInit())) {
        LOG_ERROR("Failed to initialize GLFW");
        return -1;
    }
    LOG_DEBUG("GLFW initialized");

    /* Configure OpenGL context version 3.3 */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create window and OpenGL context */
    s_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                WINDOW_TITLE, NULL, NULL);
    if (UNLIKELY(!s_window)) {
        LOG_ERROR("Failed to create window");
        glfwTerminate();
        return -1;
    }
    LOG_INFO_F("Window created (%dx%d)", WINDOW_WIDTH, WINDOW_HEIGHT);

    /* Make this window's context current */
    glfwMakeContextCurrent(s_window);

    /* Register resize callback */
    glfwSetFramebufferSizeCallback(s_window, framebuffer_size_callback);

    return 0;
}

GLFWwindow* window_get_handle(void)
{
    return s_window;
}

void poll_events(void)
{
    glfwPollEvents();
}

int window_should_close(void)
{
    return glfwWindowShouldClose(s_window);
}

void swap_buffers(void)
{
    glfwSwapBuffers(s_window);
}

void shutdown_window(void)
{
    glfwTerminate();
    LOG_DEBUG("GLFW terminated");
}
