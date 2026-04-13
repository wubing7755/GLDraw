#include <platform/window.h>

int platform_window_init(PlatformWindow* window, int width, int height, const char* title)
{
    if (!window) {
        return -1;
    }

    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window->handle = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window->handle) {
        glfwTerminate();
        return -1;
    }

    window->width = width;
    window->height = height;
    window->title = title;

    glfwMakeContextCurrent(window->handle);
    glfwSwapInterval(1);
    return 0;
}

void platform_window_shutdown(PlatformWindow* window)
{
    if (!window) {
        return;
    }

    if (window->handle) {
        glfwDestroyWindow(window->handle);
        window->handle = NULL;
    }
}

void platform_window_poll_events(void)
{
    glfwPollEvents();
}

void platform_window_swap_buffers(PlatformWindow* window)
{
    if (window && window->handle) {
        glfwSwapBuffers(window->handle);
    }
}

int platform_window_should_close(const PlatformWindow* window)
{
    if (!window || !window->handle) {
        return 1;
    }
    return glfwWindowShouldClose(window->handle);
}
