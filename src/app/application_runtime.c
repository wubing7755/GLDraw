#include <app/application_runtime.h>

#include <app/editor_controller.h>
#include <app/workspace.h>
#include <ui/ui_system.h>

#include "application_internal.h"

#include <base/math2d.h>

ToolContext application_runtime_tool_context(Application* app)
{
    return editor_controller_tool_context(app ? app->workspace : NULL);
}

int application_runtime_pointer_blocks_canvas(const Application* app,
                                              Vec2 screen_pos)
{
    if (!app) {
        return 0;
    }

    if (app->ui && ui_system_blocks_pointer(app->ui, screen_pos)) {
        return 1;
    }

    return app->ui && !ui_system_point_in_canvas(app->ui, screen_pos);
}

void application_runtime_sync_tool_pointer_anchor(Application* app)
{
    if (!app) {
        return;
    }

    editor_controller_sync_pointer_anchor(app->workspace, app->cursor_screen);
}

void application_runtime_sync_canvas_boundary(Application* app)
{
    int inside_canvas = 0;

    if (!app || editor_controller_pointer_captured(app->workspace)) {
        return;
    }

    inside_canvas = app->ui ? ui_system_point_in_canvas(app->ui, app->cursor_screen) : 1;
    if (inside_canvas != app->cursor_inside_canvas) {
        app->cursor_inside_canvas = inside_canvas;
        application_runtime_sync_tool_pointer_anchor(app);
    }
}

void application_runtime_update_canvas_viewport(Application* app)
{
    RectF viewport;
    RectF fallback_window = {0.0f, 0.0f, 1.0f, 1.0f};

    if (!app) {
        return;
    }

    if (app->ui) {
        viewport = ui_system_content_bounds(app->ui);
        fallback_window = ui_system_window_bounds(app->ui);
        workspace_set_layout(app->workspace, ui_system_layout(app->ui));
    } else {
        viewport = editor_controller_canvas_content_bounds(app->workspace);
        fallback_window.w = (float)app->window.width;
        fallback_window.h = (float)app->window.height;
        if (fallback_window.w < 1.0f) {
            fallback_window.w = 1.0f;
        }
        if (fallback_window.h < 1.0f) {
            fallback_window.h = 1.0f;
        }
    }

    if (viewport.w <= 1.0f || viewport.h <= 1.0f) {
        viewport = fallback_window;
    }

    if (app->ui) {
        editor_controller_set_canvas_background(app->workspace,
                                                ui_system_canvas_background(app->ui));
    }

    editor_controller_set_canvas_viewport(app->workspace, viewport);
    application_runtime_sync_canvas_boundary(app);
}

ToolEvent application_runtime_make_tool_event(Application* app,
                                              int button,
                                              int mods,
                                              float wheel_y)
{
    ToolEvent event;
    event = editor_controller_make_tool_event(app->workspace,
                                              app->cursor_screen,
                                              button,
                                              mods,
                                              wheel_y);
    return event;
}
