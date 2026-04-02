#include <stdio.h>
#include <GLFW/glfw3.h>
#include <core/window.h>

GLFWwindow* g_window = NULL;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
    printf("[Window] Resized to %dx%d\n", width, height);
}

int init_window(void)
{
    /* Initialize GLFW */
    if (!glfwInit())
    {
        printf("[Error] Failed to initialize GLFW\n");
        return -1;
    }
    printf("[Window] GLFW initialized\n");

    /* Configure OpenGL context version 3.3 */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create window and OpenGL context */
    g_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                WINDOW_TITLE, NULL, NULL);
    if (!g_window)
    {
        printf("[Error] Failed to create window\n");
        glfwTerminate();
        return -1;
    }
    printf("[Window] Window created (%dx%d)\n", WINDOW_WIDTH, WINDOW_HEIGHT);

    /* Make this window's context current */
    glfwMakeContextCurrent(g_window);

    /* Register resize callback */
    glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);

    return 0;
}

void poll_events(void)
{
    glfwPollEvents();
}

int window_should_close(void)
{
    return glfwWindowShouldClose(g_window);
}

void swap_buffers(void)
{
    glfwSwapBuffers(g_window);
}

void shutdown_window(void)
{
    glfwTerminate();
    printf("[Window] GLFW terminated\n");
}
