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

int platform_window_init(PlatformWindow* window, int width, int height, const char* title);
void platform_window_shutdown(PlatformWindow* window);
void platform_window_poll_events(void);
void platform_window_swap_buffers(PlatformWindow* window);
int platform_window_should_close(const PlatformWindow* window);

#endif /* GLDRAW_PLATFORM_WINDOW_H */
