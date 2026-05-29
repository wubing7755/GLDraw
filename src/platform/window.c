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
#include "platform/window_internal.h"

#include <glad/glad.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_glfw_gl3.h"

#include <stdlib.h>

static int g_glfw_initialized = 0;
static GldWindow* g_window_list = NULL;

static GldWindow* platform_window_find_native(GLFWwindow* glfw)
{
    GldWindow* current = g_window_list;

    while (current) {
        if (current->glfw == glfw) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

static void platform_framebuffer_size_dispatch(GLFWwindow* glfw, int width, int height)
{
    GldWindow* native_window = platform_window_find_native(glfw);
    int logical_width = 0;
    int logical_height = 0;

    if (!native_window || !native_window->owner) {
        return;
    }

    glfwGetWindowSize(glfw, &logical_width, &logical_height);
    native_window->owner->width = logical_width;
    native_window->owner->height = logical_height;
    native_window->owner->framebuffer_width = width;
    native_window->owner->framebuffer_height = height;
    if (native_window->framebuffer_size_callback) {
        native_window->framebuffer_size_callback(native_window->owner,
                                                 width,
                                                 height,
                                                 native_window->framebuffer_size_user_data);
    }
}

static void platform_cursor_pos_dispatch(GLFWwindow* glfw, double xpos, double ypos)
{
    GldWindow* native_window = platform_window_find_native(glfw);

    if (native_window && native_window->cursor_pos_callback) {
        native_window->cursor_pos_callback(native_window->owner,
                                           xpos,
                                           ypos,
                                           native_window->cursor_pos_user_data);
    }
}

static void platform_mouse_button_dispatch(GLFWwindow* glfw, int button, int action, int mods)
{
    GldWindow* native_window = platform_window_find_native(glfw);

    if (glfwGetWindowUserPointer(glfw)) {
        nk_glfw3_mouse_button_callback(glfw, button, action, mods);
    }
    if (native_window && native_window->mouse_button_callback) {
        native_window->mouse_button_callback(native_window->owner,
                                             button,
                                             action,
                                             mods,
                                             native_window->mouse_button_user_data);
    }
}

static void platform_key_dispatch(GLFWwindow* glfw,
                                  int key,
                                  int scancode,
                                  int action,
                                  int mods)
{
    GldWindow* native_window = platform_window_find_native(glfw);

    if (glfwGetWindowUserPointer(glfw)) {
        nk_glfw3_key_callback(glfw, key, scancode, action, mods);
    }
    if (native_window && native_window->key_callback) {
        native_window->key_callback(native_window->owner,
                                    key,
                                    scancode,
                                    action,
                                    mods,
                                    native_window->key_user_data);
    }
}

static void platform_char_dispatch(GLFWwindow* glfw, unsigned int codepoint)
{
    GldWindow* native_window = platform_window_find_native(glfw);

    if (glfwGetWindowUserPointer(glfw)) {
        nk_glfw3_char_callback(glfw, codepoint);
    }
    if (native_window && native_window->char_callback) {
        native_window->char_callback(native_window->owner,
                                     codepoint,
                                     native_window->char_user_data);
    }
}

static void platform_scroll_dispatch(GLFWwindow* glfw, double xoffset, double yoffset)
{
    GldWindow* native_window = platform_window_find_native(glfw);

    if (glfwGetWindowUserPointer(glfw)) {
        nk_gflw3_scroll_callback(glfw, xoffset, yoffset);
    }
    if (native_window && native_window->scroll_callback) {
        native_window->scroll_callback(native_window->owner,
                                       xoffset,
                                       yoffset,
                                       native_window->scroll_user_data);
    }
}

static void platform_close_dispatch(GLFWwindow* glfw)
{
    GldWindow* native_window = platform_window_find_native(glfw);

    if (native_window && native_window->close_callback) {
        native_window->close_callback(native_window->owner,
                                      native_window->close_user_data);
    }
}

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
    GldWindow* native_window = NULL;

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

    native_window = (GldWindow*)calloc(1, sizeof(*native_window));
    if (!native_window) {
        if (initialized_here) {
            glfwTerminate();
            g_glfw_initialized = 0;
        }
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    native_window->glfw = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!native_window->glfw) {
        free(native_window);
        if (initialized_here) {
            glfwTerminate();
            g_glfw_initialized = 0;
        }
        return -1;
    }

    window->handle = native_window;
    native_window->owner = window;
    native_window->next = g_window_list;
    g_window_list = native_window;
    glfwSetWindowSizeLimits(native_window->glfw, 800, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetFramebufferSizeCallback(native_window->glfw, platform_framebuffer_size_dispatch);
    glfwSetCursorPosCallback(native_window->glfw, platform_cursor_pos_dispatch);
    glfwSetMouseButtonCallback(native_window->glfw, platform_mouse_button_dispatch);
    glfwSetKeyCallback(native_window->glfw, platform_key_dispatch);
    glfwSetCharCallback(native_window->glfw, platform_char_dispatch);
    glfwSetScrollCallback(native_window->glfw, platform_scroll_dispatch);
    glfwSetWindowCloseCallback(native_window->glfw, platform_close_dispatch);

    window->width = width;
    window->height = height;
    glfwGetFramebufferSize(native_window->glfw,
                           &window->framebuffer_width,
                           &window->framebuffer_height);
    window->title = title;

    glfwMakeContextCurrent(native_window->glfw);
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
    GldWindow** current = NULL;

    if (!window) {
        return;
    }

    current = &g_window_list;
    while (*current) {
        if (*current == window->handle) {
            *current = window->handle->next;
            break;
        }
        current = &(*current)->next;
    }

    if (window->handle && window->handle->glfw) {
        glfwMakeContextCurrent(NULL);
        glfwDestroyWindow(window->handle->glfw);
        window->handle->glfw = NULL;
    }
    free(window->handle);
    window->handle = NULL;

    if (g_glfw_initialized) {
        glfwTerminate();
        g_glfw_initialized = 0;
    }
}

/**
 * @brief Poll window events.
 * @return No return value.
 */
void platform_window_poll_events(void)
{
    glfwPollEvents();
}

void platform_window_wait_events_timeout(double timeout_seconds)
{
    glfwWaitEventsTimeout(timeout_seconds);
}

/**
 * @brief Swap the window front and back buffers.
 * @param window Target window.
 * @return No return value.
 */
void platform_window_swap_buffers(PlatformWindow* window)
{
    GLFWwindow* handle = platform_window_glfw_handle(window);

    if (handle) {
        glfwSwapBuffers(handle);
    }
}

/**
 * @brief Query whether the window should close.
 * @param window Target window.
 * @return Non-zero if the window should close, zero otherwise.
 */
int platform_window_should_close(const PlatformWindow* window)
{
    GLFWwindow* handle = platform_window_glfw_handle(window);

    if (!handle) {
        return 1;
    }
    return glfwWindowShouldClose(handle);
}

void platform_window_set_should_close(PlatformWindow* window, int should_close)
{
    GLFWwindow* handle = platform_window_glfw_handle(window);

    if (handle) {
        glfwSetWindowShouldClose(handle, should_close ? GLFW_TRUE : GLFW_FALSE);
    }
}

void platform_window_get_size(PlatformWindow* window, int* width, int* height)
{
    GLFWwindow* handle = platform_window_glfw_handle(window);

    if (!window) {
        return;
    }

    if (handle) {
        glfwGetWindowSize(handle, &window->width, &window->height);
    }
    if (width) {
        *width = window->width;
    }
    if (height) {
        *height = window->height;
    }
}

void platform_window_get_framebuffer_size(PlatformWindow* window, int* width, int* height)
{
    GLFWwindow* handle = platform_window_glfw_handle(window);

    if (!window) {
        return;
    }

    if (handle) {
        glfwGetFramebufferSize(handle,
                               &window->framebuffer_width,
                               &window->framebuffer_height);
    }
    if (width) {
        *width = window->framebuffer_width;
    }
    if (height) {
        *height = window->framebuffer_height;
    }
}

double platform_time_seconds(void)
{
    return glfwGetTime();
}

void platform_window_on_framebuffer_size(PlatformWindow* window,
                                         PlatformFramebufferSizeCallback callback,
                                         void* user_data)
{
    if (!window || !window->handle) {
        return;
    }

    window->handle->framebuffer_size_callback = callback;
    window->handle->framebuffer_size_user_data = user_data;
}

void platform_window_on_cursor_pos(PlatformWindow* window,
                                   PlatformCursorPosCallback callback,
                                   void* user_data)
{
    if (!window || !window->handle) {
        return;
    }

    window->handle->cursor_pos_callback = callback;
    window->handle->cursor_pos_user_data = user_data;
}

void platform_window_on_mouse_button(PlatformWindow* window,
                                     PlatformMouseButtonCallback callback,
                                     void* user_data)
{
    if (!window || !window->handle) {
        return;
    }

    window->handle->mouse_button_callback = callback;
    window->handle->mouse_button_user_data = user_data;
}

void platform_window_on_key(PlatformWindow* window,
                            PlatformKeyCallback callback,
                            void* user_data)
{
    if (!window || !window->handle) {
        return;
    }

    window->handle->key_callback = callback;
    window->handle->key_user_data = user_data;
}

void platform_window_on_char(PlatformWindow* window,
                             PlatformCharCallback callback,
                             void* user_data)
{
    if (!window || !window->handle) {
        return;
    }

    window->handle->char_callback = callback;
    window->handle->char_user_data = user_data;
}

void platform_window_on_scroll(PlatformWindow* window,
                               PlatformScrollCallback callback,
                               void* user_data)
{
    if (!window || !window->handle) {
        return;
    }

    window->handle->scroll_callback = callback;
    window->handle->scroll_user_data = user_data;
}

void platform_window_on_close(PlatformWindow* window,
                              PlatformCloseCallback callback,
                              void* user_data)
{
    if (!window || !window->handle) {
        return;
    }

    window->handle->close_callback = callback;
    window->handle->close_user_data = user_data;
}

struct nk_context* platform_window_nuklear_init(struct nk_glfw* glfw, PlatformWindow* window)
{
    return (glfw && window) ?
               nk_glfw3_init(glfw, platform_window_glfw_handle(window), NK_GLFW3_DEFAULT) :
               NULL;
}

void platform_window_nuklear_shutdown(struct nk_glfw* glfw)
{
    if (glfw) {
        nk_glfw3_shutdown(glfw);
    }
}

void platform_window_nuklear_font_stash_begin(struct nk_glfw* glfw,
                                              struct nk_font_atlas** atlas)
{
    if (glfw) {
        nk_glfw3_font_stash_begin(glfw, atlas);
    }
}

void platform_window_nuklear_font_stash_end(struct nk_glfw* glfw)
{
    if (glfw) {
        nk_glfw3_font_stash_end(glfw);
    }
}

void platform_window_nuklear_new_frame(struct nk_glfw* glfw)
{
    if (glfw) {
        nk_glfw3_new_frame(glfw);
    }
}

void platform_window_nuklear_render(struct nk_glfw* glfw,
                                    int anti_aliasing,
                                    int max_vertex_buffer,
                                    int max_element_buffer)
{
    if (glfw) {
        nk_glfw3_render(glfw,
                        (enum nk_anti_aliasing)anti_aliasing,
                        max_vertex_buffer,
                        max_element_buffer);
    }
}
