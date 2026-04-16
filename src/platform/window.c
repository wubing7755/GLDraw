/**
 * @file window.c
 * @brief GLFW window lifecycle wrapper implementation.
 *
 * Role in project:
 * - Initializes OpenGL-compatible GLFW window/context.
 * - Provides small wrapper calls used by the app loop.
 *
 * Module relationships:
 * - Called by `application.c`.
 * - Provides valid context required by renderer/UI init.
 */
#include <platform/window.h>

/** Initialize GLFW and create window/context. Returns 0 on success, -1 on failure. */
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

    glfwSetWindowSizeLimits(window->handle, 800, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);

    window->width = width;
    window->height = height;
    window->title = title;

    glfwMakeContextCurrent(window->handle);
    glfwSwapInterval(1);
    return 0;
}

/** Destroy window handle if it exists. */
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

/** Poll pending OS events. */
void platform_window_poll_events(void)
{
    glfwPollEvents();
}

/** Swap front/back buffers for valid window handle. */
void platform_window_swap_buffers(PlatformWindow* window)
{
    if (window && window->handle) {
        glfwSwapBuffers(window->handle);
    }
}

/** Return close-state (treat invalid window as closed). */
int platform_window_should_close(const PlatformWindow* window)
{
    if (!window || !window->handle) {
        return 1;
    }
    return glfwWindowShouldClose(window->handle);
}
