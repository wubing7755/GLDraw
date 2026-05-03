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
};

static inline GLFWwindow* platform_window_glfw_handle(const PlatformWindow* window)
{
    return (window && window->handle) ? window->handle->glfw : NULL;
}

#endif /* GLDRAW_PLATFORM_WINDOW_INTERNAL_H */
