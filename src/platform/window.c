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

/**
 * @brief Initialize GLFW and create an OpenGL 3.3 Core Profile window and context.
 * @param window [in,out] Window structure to populate.
 * @param width [in] Requested window width in pixels.
 * @param height [in] Requested window height in pixels.
 * @param title [in] Window title string.
 * @return `0` on success; `-1` on GLFW init failure or window creation failure.
 *
 * Error conditions:
 * - Returns `-1` if `window` is `NULL`.
 * - Returns `-1` if GLFW fails to initialize.
 * - Returns `-1` if window or OpenGL context creation fails.
 *
 * Side effects:
 * - Makes the OpenGL context current for the created window.
 * - Sets swap interval to 1 (vsync enabled).
 * - Enforces a minimum window size of 800×600 pixels.
 */
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

/**
 * @brief Destroy window handle if present.
 * @param window [in,out] Window to destroy; safe no-op when `NULL`.
 * @return None.
 *
 * Note:
 * - Does NOT call `glfwTerminate()`; GLFW lifetime is managed separately.
 * - Safe to call multiple times (idempotent after first call).
 */
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
