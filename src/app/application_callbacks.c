#include "application_internal.h"

#include <app/application_runtime.h>
#include <app/editor_controller.h>
#include <app/workspace_actions.h>
#include <base/math2d.h>
#include <input/input_router.h>

#include <GLFW/glfw3.h>

static void framebuffer_size_callback(PlatformWindow* window,
                                      int width,
                                      int height,
                                      void* user_data)
{
    Application* app = (Application*)user_data;
    int window_width = 0;
    int window_height = 0;

    if (!app || !window) {
        return;
    }

    platform_window_get_size(window, &window_width, &window_height);
    application_runtime_update_canvas_viewport(app);
    render_system_resize(app->renderer, window_width, window_height, width, height);
}

static void cursor_pos_callback(PlatformWindow* window,
                                double xpos,
                                double ypos,
                                void* user_data)
{
    Application* app = (Application*)user_data;
    ToolContext context;
    ToolEvent event;
    (void)window;

    if (!app) {
        return;
    }

    app->cursor_screen = vec2_make((float)xpos, (float)ypos);
    application_runtime_sync_canvas_boundary(app);

    if (!editor_controller_pointer_captured(app->workspace) &&
        application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
        return;
    }

    context = application_runtime_tool_context(app);
    event = application_runtime_make_tool_event(app, -1, 0, 0.0f);

    editor_controller_pointer_move(app->workspace, &context, &event);
}

static void mouse_button_callback(PlatformWindow* window,
                                  int button,
                                  int action,
                                  int mods,
                                  void* user_data)
{
    Application* app = (Application*)user_data;
    ToolContext context;
    ToolEvent event;
    (void)window;

    if (!app) {
        return;
    }

    if (action == GLFW_PRESS) {
        if (app->ui &&
            ui_system_handle_mouse_button(app->ui, app->cursor_screen, button, action)) {
            return;
        }
        if (!editor_controller_pointer_captured(app->workspace) &&
            application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
            return;
        }
        context = application_runtime_tool_context(app);
        event = application_runtime_make_tool_event(app, button, mods, 0.0f);
        editor_controller_pointer_down(app->workspace, &context, &event);
    } else if (action == GLFW_RELEASE) {
        /* Ignore release outside canvas unless pointer capture is active. */
        if (!editor_controller_pointer_captured(app->workspace)) {
            if (application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
                return;
            }
            return;
        }
        context = application_runtime_tool_context(app);
        event = application_runtime_make_tool_event(app, button, mods, 0.0f);
        editor_controller_pointer_up(app->workspace, &context, &event);
    }
}

static void key_callback(PlatformWindow* window,
                         int key,
                         int scancode,
                         int action,
                         int mods,
                         void* user_data)
{
    Application* app = (Application*)user_data;
    ToolContext context;
    InputRouterContext router_context;
    KeyEvent event;
    (void)window;
    (void)scancode;

    if (!app) {
        return;
    }

    if (app->ui && ui_system_handle_key(app->ui, key, action)) {
        return;
    }

    context = application_runtime_tool_context(app);
    router_context.workspace = app->workspace;
    router_context.tool_context = &context;
    router_context.ui_has_keyboard_focus =
        app->ui ? ui_system_has_active_interaction(app->ui) : 0;
    event.key = key;
    event.mods = mods;
    event.action = action;
    event.repeated = (action == GLFW_REPEAT);
    input_router_handle_key(&router_context, &event);
}

static void scroll_callback(PlatformWindow* window,
                            double xoffset,
                            double yoffset,
                            void* user_data)
{
    Application* app = (Application*)user_data;
    ToolContext context;
    (void)window;
    (void)xoffset;

    if (!app) {
        return;
    }

    if (!editor_controller_pointer_captured(app->workspace) &&
        application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
        return;
    }

    context = application_runtime_tool_context(app);
    editor_controller_scroll(app->workspace, &context, app->cursor_screen, (float)yoffset);
}

static void window_close_callback(PlatformWindow* window, void* user_data)
{
    Application* app = (Application*)user_data;

    if (!app || !window) {
        return;
    }

    platform_window_set_should_close(window, 0);
    workspace_request_action(app->workspace, WORKSPACE_ACTION_EXIT_APPLICATION);
}

void application_register_platform_callbacks(Application* app)
{
    if (!app) {
        return;
    }

    platform_window_on_framebuffer_size(&app->window, framebuffer_size_callback, app);
    platform_window_on_cursor_pos(&app->window, cursor_pos_callback, app);
    platform_window_on_mouse_button(&app->window, mouse_button_callback, app);
    platform_window_on_key(&app->window, key_callback, app);
    platform_window_on_scroll(&app->window, scroll_callback, app);
    platform_window_on_close(&app->window, window_close_callback, app);
}
