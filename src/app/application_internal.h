#ifndef GLDRAW_APP_APPLICATION_INTERNAL_H
#define GLDRAW_APP_APPLICATION_INTERNAL_H

#include <app/command_dispatcher.h>
#include <app/workspace_internal.h>
#include <platform/window.h>
#include <render/render_system.h>
#include <ui/editor_viewmodel.h>
#include <ui/ui_system.h>

typedef struct Application {
    PlatformWindow window;
    Workspace workspace;
    CommandDispatcher dispatcher;
    RenderSystem* renderer;
    UiSystem* ui;
    EditorViewModel view_model;
    Vec2 cursor_screen;
    int cursor_inside_canvas;
    int pending_export_png;
    char pending_export_png_path[GLDRAW_PATH_MAX];
} Application;

#endif /* GLDRAW_APP_APPLICATION_INTERNAL_H */
