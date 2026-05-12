/**
 * @file window_internal.h
 * @brief Private GLFW-backed platform window details.
 */
#ifndef GLDRAW_PLATFORM_WINDOW_INTERNAL_H
#define GLDRAW_PLATFORM_WINDOW_INTERNAL_H

#include <platform/window.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

struct GldWindow {
    GLFWwindow* glfw;
    PlatformWindow* owner;
    PlatformFramebufferSizeCallback framebuffer_size_callback;
    void* framebuffer_size_user_data;
    PlatformCursorPosCallback cursor_pos_callback;
    void* cursor_pos_user_data;
    PlatformMouseButtonCallback mouse_button_callback;
    void* mouse_button_user_data;
    PlatformKeyCallback key_callback;
    void* key_user_data;
    PlatformScrollCallback scroll_callback;
    void* scroll_user_data;
    PlatformCloseCallback close_callback;
    void* close_user_data;
    struct GldWindow* next;
};

static inline GLFWwindow* platform_window_glfw_handle(const PlatformWindow* window)
{
    return (window && window->handle) ? window->handle->glfw : NULL;
}

#endif /* GLDRAW_PLATFORM_WINDOW_INTERNAL_H */
