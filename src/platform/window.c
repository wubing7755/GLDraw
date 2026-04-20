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

static int g_glfw_initialized = 0;

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
    int initialized_here = 0;

    if (!window) {
        return -1;
    }

    if (!g_glfw_initialized) {
        if (!glfwInit()) {
            return -1;
        }
        g_glfw_initialized = 1;
        initialized_here = 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window->handle = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window->handle) {
        if (initialized_here) {
            glfwTerminate();
            g_glfw_initialized = 0;
        }
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
 * - Also terminates GLFW after destroying the final window for this process.
 * - Safe to call multiple times (idempotent after first call).
 */
void platform_window_shutdown(PlatformWindow* window)
{
    if (!window) {
        return;
    }

    if (window->handle) {
        glfwMakeContextCurrent(NULL);
        glfwDestroyWindow(window->handle);
        window->handle = NULL;
    }

    if (g_glfw_initialized) {
        glfwTerminate();
        g_glfw_initialized = 0;
    }
}

/**
 * @brief platform_window_poll_events 函数。
 *
 * @param void 无参数。
 * @return 无。
 */
void platform_window_poll_events(void)
{
    glfwPollEvents();
}

/**
 * @brief platform_window_swap_buffers 函数。
 *
 * @param window 参数 `window`。
 * @return 无。
 */
void platform_window_swap_buffers(PlatformWindow* window)
{
    if (window && window->handle) {
        glfwSwapBuffers(window->handle);
    }
}

/**
 * @brief platform_window_should_close 函数。
 *
 * @param window 参数 `window`。
 * @return 函数返回值。
 */
int platform_window_should_close(const PlatformWindow* window)
{
    if (!window || !window->handle) {
        return 1;
    }
    return glfwWindowShouldClose(window->handle);
}
