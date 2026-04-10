#include <app/application.h>

#include <app/workspace.h>
#include <base/log.h>
#include <base/math2d.h>
#include <document/persistence.h>
#include <platform/window.h>
#include <render/render_system.h>
#include <ui/ui_system.h>

#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <string.h>

typedef struct {
    PlatformWindow window;
    Workspace workspace;
    RenderSystem* renderer;
    UiSystem* ui;
    Vec2 cursor_screen;
} Application;

static int app_workspace_save(Workspace* workspace, void* user_data);
static int app_workspace_load(Workspace* workspace, void* user_data);

static const char* app_default_document_path(void)
{
    return "document.json";
}

static const char* app_current_document_path(const Application* app)
{
    if (app->workspace.current_document_path[0] != '\0') {
        return app->workspace.current_document_path;
    }
    return app_default_document_path();
}

static void app_set_document_path(Application* app, const char* path)
{
    if (!app || !path) {
        return;
    }

    strncpy(app->workspace.current_document_path,
            path,
            sizeof(app->workspace.current_document_path) - 1u);
    app->workspace.current_document_path[sizeof(app->workspace.current_document_path) - 1u] = '\0';
}

static void app_reset_tool_state(Application* app)
{
    if (!app) {
        return;
    }

    tool_controller_shutdown(&app->workspace.tools);
    tool_controller_init(&app->workspace.tools);
}

static int app_save_document(Application* app)
{
    const char* path = app_current_document_path(app);

    if (!document_save_json(&app->workspace.document, path)) {
        LOG_ERROR("%s", "Save document failed");
        return 0;
    }

    app_set_document_path(app, path);
    workspace_mark_saved(&app->workspace);
    LOG_INFO("Saved document: %s", path);
    return 1;
}

static int app_load_document(Application* app)
{
    const char* path = app_current_document_path(app);

    if (!document_load_json(&app->workspace.document, path)) {
        LOG_ERROR("%s", "Load document failed");
        return 0;
    }

    document_history_shutdown(&app->workspace.history);
    document_history_init(&app->workspace.history);
    app_reset_tool_state(app);
    app_set_document_path(app, path);
    workspace_mark_saved(&app->workspace);
    LOG_INFO("Loaded document: %s", path);
    return 1;
}

static int app_workspace_save(Workspace* workspace, void* user_data)
{
    (void)workspace;
    return app_save_document((Application*)user_data);
}

static int app_workspace_load(Workspace* workspace, void* user_data)
{
    (void)workspace;
    return app_load_document((Application*)user_data);
}

static ToolContext app_tool_context(Application* app)
{
    ToolContext context;
    context.workspace = &app->workspace;
    context.document = &app->workspace.document;
    context.history = &app->workspace.history;
    context.canvas = &app->workspace.canvas;
    return context;
}

static void update_canvas_viewport(Application* app)
{
    RectF viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.w = (float)app->window.width;
    viewport.h = (float)app->window.height;
    canvas_view_set_viewport(&app->workspace.canvas, viewport);
}

static ToolEvent make_tool_event(Application* app, int button, int mods, float wheel_y)
{
    ToolEvent event;
    event.screen_pos = app->cursor_screen;
    event.world_pos = canvas_view_screen_to_world(&app->workspace.canvas, app->cursor_screen);
    event.delta_screen = vec2_sub(event.screen_pos, app->workspace.tools.last_screen);
    event.delta_world = vec2_sub(event.world_pos, app->workspace.tools.last_world);
    event.button = button;
    event.mods = mods;
    event.wheel_y = wheel_y;
    return event;
}

static void framebuffer_size_callback(GLFWwindow* handle, int width, int height)
{
    Application* app = (Application*)glfwGetWindowUserPointer(handle);
    if (!app) {
        return;
    }
    app->window.width = width;
    app->window.height = height;
    update_canvas_viewport(app);
    render_system_resize(app->renderer, width, height);
}

static void cursor_pos_callback(GLFWwindow* handle, double xpos, double ypos)
{
    Application* app = (Application*)glfwGetWindowUserPointer(handle);
    ToolContext context;
    ToolEvent event;

    if (!app) {
        return;
    }

    app->cursor_screen = vec2_make((float)xpos, (float)ypos);
    context = app_tool_context(app);
    event = make_tool_event(app, -1, 0, 0.0f);

    if (!app->workspace.tools.pointer_captured &&
        ui_system_blocks_pointer(app->ui, app->cursor_screen)) {
        return;
    }

    tool_controller_pointer_move(&app->workspace.tools, &context, &event);
}

static void mouse_button_callback(GLFWwindow* handle, int button, int action, int mods)
{
    Application* app = (Application*)glfwGetWindowUserPointer(handle);
    ToolContext context;
    ToolEvent event;

    if (!app) {
        return;
    }

    context = app_tool_context(app);
    event = make_tool_event(app, button, mods, 0.0f);

    if (action == GLFW_PRESS) {
        if (!app->workspace.tools.pointer_captured &&
            ui_system_blocks_pointer(app->ui, app->cursor_screen)) {
            return;
        }
        tool_controller_pointer_down(&app->workspace.tools, &context, &event);
    } else if (action == GLFW_RELEASE) {
        tool_controller_pointer_up(&app->workspace.tools, &context, &event);
    }
}

static void key_callback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
    Application* app = (Application*)glfwGetWindowUserPointer(handle);
    ToolContext context;
    (void)scancode;

    if (!app || action != GLFW_PRESS) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE && app->workspace.tools.active_kind == TOOL_KIND_SELECT) {
        glfwSetWindowShouldClose(handle, GLFW_TRUE);
        return;
    }

    if ((mods & GLFW_MOD_CONTROL) != 0 && key == GLFW_KEY_S) {
        app_save_document(app);
        return;
    }

    if ((mods & GLFW_MOD_CONTROL) != 0 && key == GLFW_KEY_O) {
        app_load_document(app);
        return;
    }

    context = app_tool_context(app);
    tool_controller_key_down(&app->workspace.tools, &context, key, mods);
}

static void scroll_callback(GLFWwindow* handle, double xoffset, double yoffset)
{
    Application* app = (Application*)glfwGetWindowUserPointer(handle);
    ToolContext context;
    (void)xoffset;

    if (!app) {
        return;
    }

    if (ui_system_blocks_pointer(app->ui, app->cursor_screen)) {
        return;
    }

    context = app_tool_context(app);
    tool_controller_scroll(&app->workspace.tools, &context, app->cursor_screen, (float)yoffset);
}

static int app_init(Application* app)
{
    RectF viewport = {0.0f, 0.0f, 1440.0f, 900.0f};

    if (platform_window_init(&app->window, 1440, 900, "GLDraw Canvas") != 0) {
        LOG_ERROR("%s", "Failed to create window");
        return -1;
    }

    document_init(&app->workspace.document);
    document_history_init(&app->workspace.history);
    canvas_view_init(&app->workspace.canvas, &app->workspace.document, viewport);
    tool_controller_init(&app->workspace.tools);
    app_set_document_path(app, app_default_document_path());
    app->workspace.save_document = app_workspace_save;
    app->workspace.load_document = app_workspace_load;
    app->workspace.command_user_data = app;
    workspace_mark_saved(&app->workspace);

    app->renderer = render_system_create(&app->window);
    if (!app->renderer) {
        LOG_ERROR("%s", "Failed to initialize renderer");
        return -1;
    }

    app->ui = ui_system_create(&app->window);
    if (!app->ui) {
        LOG_ERROR("%s", "Failed to initialize UI");
        return -1;
    }

    glfwSetWindowUserPointer(app->window.handle, app);
    glfwSetFramebufferSizeCallback(app->window.handle, framebuffer_size_callback);
    glfwSetCursorPosCallback(app->window.handle, cursor_pos_callback);
    glfwSetMouseButtonCallback(app->window.handle, mouse_button_callback);
    glfwSetKeyCallback(app->window.handle, key_callback);
    glfwSetScrollCallback(app->window.handle, scroll_callback);

    render_system_resize(app->renderer, app->window.width, app->window.height);
    update_canvas_viewport(app);
    app->cursor_screen = vec2_make(app->window.width * 0.5f, app->window.height * 0.5f);
    return 0;
}

static void app_shutdown(Application* app)
{
    if (!app) {
        return;
    }

    ui_system_destroy(app->ui);
    render_system_destroy(app->renderer);
    tool_controller_shutdown(&app->workspace.tools);
    document_history_shutdown(&app->workspace.history);
    document_shutdown(&app->workspace.document);
    platform_window_shutdown(&app->window);
}

int app_run(void)
{
    Application* app = (Application*)calloc(1, sizeof(*app));

    if (!app) {
        LOG_ERROR("%s", "Failed to allocate application state");
        return -1;
    }

    if (app_init(app) != 0) {
        app_shutdown(app);
        free(app);
        return -1;
    }

    while (!platform_window_should_close(&app->window)) {
        platform_window_poll_events();
        ui_system_begin_frame(app->ui);
        ui_system_build(app->ui, &app->workspace);
        render_system_draw(app->renderer,
                           &app->workspace.document,
                           &app->workspace.canvas,
                           tool_controller_overlay_object(&app->workspace.tools));
        ui_system_render(app->ui);
        platform_window_swap_buffers(&app->window);
    }

    app_shutdown(app);
    free(app);
    return 0;
}
