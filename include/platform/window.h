/**
 * @file window.h
 * @brief Thin GLFW window lifecycle wrapper.
 *
 * Role in project:
 * - Encapsulates window/context creation and basic event/buffer calls.
 * - Keeps application code independent from raw GLFW setup details.
 *
 * Module relationships:
 * - Used by application startup loop and renderer initialization.
 */
#ifndef GLDRAW_PLATFORM_WINDOW_H
#define GLDRAW_PLATFORM_WINDOW_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

typedef struct {
    GLFWwindow* handle;
    int width;
    int height;
    const char* title;
} PlatformWindow;

/** Initialize GLFW and create an OpenGL 3.3 core window. Returns 0 on success. */
int platform_window_init(PlatformWindow* window, int width, int height, const char* title);
/** Destroy window handle if present. */
void platform_window_shutdown(PlatformWindow* window);
/** Poll pending OS/window events. */
void platform_window_poll_events(void);
/** Present current backbuffer if window is valid. */
void platform_window_swap_buffers(PlatformWindow* window);
/** Return non-zero when window is invalid or close was requested. */
int platform_window_should_close(const PlatformWindow* window);

#endif /* GLDRAW_PLATFORM_WINDOW_H */
