/**
 * @file window.h
 * @brief GLFW window thin-wrapper interface.
 */
#ifndef GLDRAW_PLATFORM_WINDOW_H
#define GLDRAW_PLATFORM_WINDOW_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/**
 * @struct PlatformWindow
 * @brief Platform window state.
 *
 * @member handle GLFW native window handle.
 * @member width Desired window width.
 * @member height Desired window height.
 * @member title Window title string.
 */
typedef struct {
    GLFWwindow* handle;
    int width;
    int height;
    const char* title;
} PlatformWindow;

/**
 * @brief Initialize GLFW and create the window.
 * @param window Output address for the window structure.
 * @param width Target width.
 * @param height Target height.
 * @param title Window title.
 * @return `0` on success, `-1` on failure.
 */
int platform_window_init(PlatformWindow* window, int width, int height, const char* title);

/**
 * @brief Destroy the window and shut down GLFW.
 * @param window Target window.
 * @return No return value.
 */
void platform_window_shutdown(PlatformWindow* window);

/**
 * @brief Poll window events.
 * @return No return value.
 */
void platform_window_poll_events(void);

/**
 * @brief Swap the window front and back buffers.
 * @param window Target window.
 * @return No return value.
 */
void platform_window_swap_buffers(PlatformWindow* window);

/**
 * @brief Query whether the window should close.
 * @param window Target window.
 * @return Non-zero if the window should close, zero otherwise.
 */
int platform_window_should_close(const PlatformWindow* window);

#endif /* GLDRAW_PLATFORM_WINDOW_H */
