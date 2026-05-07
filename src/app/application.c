/**
 * @file application.c
 * @brief Runtime bootstrap, event wiring, and main loop orchestration.
 *
 * Role in project:
 * - Owns application lifetime from initialization to shutdown.
 * - Bridges GLFW events into tool/document/canvas operations.
 *
 * Module relationships:
 * - Uses workspace as central state container.
 * - Coordinates platform window, render system, UI system, tools, and persistence.
 */
#include <app/application.h>

#include <app/command_registry.h>
#include <app/command_dispatcher.h>
#include <app/workspace_actions.h>
#include <app/workspace_dialogs.h>
#include <app/workspace.h>
#include <app/workspace_service.h>
#include <base/log.h>
#include <base/math2d.h>
#include <base/path_utils.h>
#include <document/persistence.h>
#include <input/input_router.h>
#include <platform/file_dialog.h>
#include <platform/window.h>
#include "platform/window_internal.h"
#include <render/render_device_factory.h>
#include <render/render_system.h>
#include <ui/ui_menu_actions.h>
#include <ui/ui_menu_def.h>
#include <ui/editor_viewmodel.h>
#include <ui/ui_system.h>

#include <GLFW/glfw3.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_KEYMAP_SETTINGS_PATH "gldraw.keymap.json"

typedef struct {
  PlatformWindow window;
  Workspace workspace;
  CommandDispatcher dispatcher;
  RenderSystem *renderer;
  UiSystem *ui;
  EditorViewModel view_model;
  Vec2 cursor_screen;
  int cursor_inside_canvas;
  int pending_export_png;
  char pending_export_png_path[GLDRAW_PATH_MAX];
} Application;

static int app_workspace_save(Workspace *workspace, void *user_data);
static int app_workspace_save_as(Workspace *workspace, void *user_data);
static int app_workspace_export_png(Workspace *workspace, void *user_data);
static int app_workspace_load(Workspace *workspace, void *user_data);

static int app_workspace_execute_action(Workspace *workspace,
                                        WorkspaceActionType action,
                                        void *user_data);

/**
 * @brief Write formatted status text into workspace status buffer.
 * @param app [in,out] Application state.
 * @param fmt [in] `printf`-style format string.
 * @param ... [in] Format arguments.
 * @return None.
 *
 * Edge cases:
 * - Safe no-op when `app` or `fmt` is null.
 *
 * Risk note:
 * - Uses `vsnprintf` with explicit buffer size to avoid overflow.
 * Time complexity: `O(L)` where `L` is formatted output length.
 */
static void app_set_status(Application *app, const char *fmt, ...) {
  va_list args;
  char message[256];

  if (!app || !fmt) {
    return;
  }

  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  workspace_set_status_message(&app->workspace, message);
}

/**
 * @brief Check if a file exists at the given path.
 * @param path File path string.
 * @return `1` if the file exists, `0` otherwise.
 */
static int app_file_exists(const char *path) {
  return workspace_service_file_exists(path);
}

static const char *app_current_document_path(const Application *app) {
  return workspace_service_document_path(&app->workspace);
}

static void app_update_save_as_dialog_message(Application *app,
                                              const char *error_text) {
  char directory[GLDRAW_PATH_MAX];

  if (!app) {
    return;
  }

  if (!path_utils_dirname(app_current_document_path(app), directory, sizeof(directory))) {
    snprintf(directory, sizeof(directory), ".");
  }
  if (error_text && error_text[0] != '\0') {
    char message[1024];

    snprintf(message,
             sizeof(message),
             "%s\n\nEnter a new filename.\nThe file will be saved in the same directory:\n%s",
             error_text,
             directory);
    workspace_dialog_set_message(&app->workspace, message);
    return;
  }

  {
    char message[1024];

    snprintf(message,
             sizeof(message),
             "Enter a new filename.\nThe file will be saved in the same directory:\n%s",
             directory);
    workspace_dialog_set_message(&app->workspace, message);
  }
}

/**
 * @brief Copy document path into workspace path buffer.
 * @param app [in,out] Application state.
 * @param path [in] Source path string.
 * @return None.
 *
 * Risk note:
 * - Uses bounded copy and forced NUL termination.
 * Time complexity: `O(min(len(path), capacity))`.
 */
static void app_set_document_path(Application *app, const char *path) {
  workspace_service_set_document_path(&app->workspace, path);
}

static void app_suggest_png_export_filename(const Application *app,
                                            char *buffer,
                                            size_t buffer_size) {
  const char *basename = path_utils_basename_or_default(app_current_document_path(app),
                                                        "document.json");
  size_t length = 0u;

  if (!buffer || buffer_size == 0u) {
    return;
  }

  buffer[0] = '\0';
  length = strlen(basename);
  if (length > 5u && path_utils_has_extension(basename, ".json")) {
    length -= 5u;
  }
  if (length == 0u) {
    snprintf(buffer, buffer_size, "document.png");
    return;
  }
  if (length + 5u >= buffer_size) {
    snprintf(buffer, buffer_size, "document.png");
    return;
  }

  memcpy(buffer, basename, length);
  memcpy(buffer + length, ".png", 5u);
}

static int app_copy_png_export_path(const char *selected_path,
                                    char *output,
                                    size_t output_size) {
  size_t path_length = 0u;

  if (!selected_path || !output || output_size == 0u || selected_path[0] == '\0') {
    return 0;
  }

  path_length = strlen(selected_path);
  if (path_utils_has_extension(selected_path, ".png")) {
    if (path_length + 1u > output_size) {
      return 0;
    }
    memcpy(output, selected_path, path_length + 1u);
    return 1;
  }

  if (path_length + 5u > output_size) {
    return 0;
  }

  memcpy(output, selected_path, path_length);
  memcpy(output + path_length, ".png", 5u);
  return 1;
}

static int app_new_document(Application *app) {
  if (!app) {
    return 0;
  }

  return workspace_service_new_document(&app->workspace);
}

static int app_save_document_to_path(Application *app, const char *path) {
  return workspace_service_save_to_path(&app->workspace, path);
}

static int app_save_document(Application *app) {
  return workspace_service_save(&app->workspace);
}

static int app_save_as_document(Application *app) {
  char filename[GLDRAW_PATH_MAX];
  char target_path[GLDRAW_PATH_MAX];
  const char *input = NULL;

  if (!app) {
    return 0;
  }

  input = workspace_dialog_input_text(&app->workspace);
  if (!path_utils_copy_trimmed(input, filename, sizeof(filename)) ||
      !path_utils_is_safe_filename(filename)) {
    app_set_status(app, "Save As failed: invalid filename");
    app_update_save_as_dialog_message(app, "Invalid filename. Use a simple file name without path separators or reserved characters.");
    return 0;
  }

  if (!path_utils_join_same_directory(app_current_document_path(app),
                                      filename,
                                      ".json",
                                      target_path,
                                      sizeof(target_path))) {
    app_set_status(app, "Save As failed: path too long");
    app_update_save_as_dialog_message(app, "Invalid filename. The resulting path is too long.");
    return 0;
  }

  return app_save_document_to_path(app, target_path);
}

static int app_load_document_from_path(Application *app, const char *path) {
  if (!app) {
    return 0;
  }

  return workspace_service_load_from_path(&app->workspace, path);
}

static int app_load_document(Application *app) {
  return workspace_service_load(&app->workspace);
}

static int app_open_document_with_picker(Application *app) {
  char selected_path[GLDRAW_PATH_MAX];
  PlatformFileDialogResult result;

  if (!app) {
    return 0;
  }

  result = platform_file_dialog_open_document(selected_path, sizeof(selected_path));
  if (result == PLATFORM_FILE_DIALOG_CANCELLED) {
    app_set_status(app, "Open cancelled.");
    return 1;
  }
  if (result == PLATFORM_FILE_DIALOG_ERROR) {
    app_set_status(app, "Open failed: file picker unavailable or failed.");
    return 0;
  }

  return app_load_document_from_path(app, selected_path);
}

static int app_request_export_png(Application *app) {
  char suggested_filename[GLDRAW_PATH_MAX];
  char selected_path[GLDRAW_PATH_MAX];
  PlatformFileDialogResult result;

  if (!app) {
    return 0;
  }

  app_suggest_png_export_filename(app, suggested_filename, sizeof(suggested_filename));
  result = platform_file_dialog_save_png(selected_path,
                                         sizeof(selected_path),
                                         suggested_filename);
  if (result == PLATFORM_FILE_DIALOG_CANCELLED) {
    app_set_status(app, "Export PNG cancelled.");
    return 1;
  }
  if (result == PLATFORM_FILE_DIALOG_ERROR) {
    app_set_status(app, "Export PNG failed: save dialog unavailable or failed.");
    return 0;
  }

  if (!app_copy_png_export_path(selected_path,
                                app->pending_export_png_path,
                                sizeof(app->pending_export_png_path))) {
    app_set_status(app, "Export PNG failed: path too long.");
    return 0;
  }

  app->pending_export_png = 1;
  app_set_status(app, "Export PNG queued: %s", app->pending_export_png_path);
  return 1;
}

static void app_flush_pending_export_png(Application *app) {
  char export_path[GLDRAW_PATH_MAX];

  if (!app || !app->pending_export_png) {
    return;
  }

  snprintf(export_path, sizeof(export_path), "%s", app->pending_export_png_path);
  app->pending_export_png = 0;
  app->pending_export_png_path[0] = '\0';

  if (render_system_export_png(app->renderer,
                               &app->workspace.core.canvas,
                               export_path)) {
    app_set_status(app, "Exported PNG: %s", export_path);
    LOG_INFO("Exported PNG: %s", export_path);
    return;
  }

  app_set_status(app, "Export PNG failed: %s", export_path);
  LOG_ERROR("Export PNG failed: %s", export_path);
}

static int app_exit_application(Application *app) {
  GLFWwindow *handle = app ? platform_window_glfw_handle(&app->window) : NULL;

  if (!app || !handle) {
    return 0;
  }

  glfwSetWindowShouldClose(handle, GLFW_TRUE);
  app_set_status(app, "Closing application");
  return 1;
}

/**
 * @brief Workspace save callback.
 * @param workspace Workspace (unused, accessed via user_data).
 * @param user_data Application instance.
 * @return `1` on success, `0` on failure.
 */
static int app_workspace_save(Workspace *workspace, void *user_data) {
  (void)workspace;
  return app_save_document((Application *)user_data);
}

static int app_workspace_save_as(Workspace *workspace, void *user_data) {
  (void)workspace;
  return app_save_as_document((Application *)user_data);
}

static int app_workspace_export_png(Workspace *workspace, void *user_data) {
  (void)workspace;
  return app_request_export_png((Application *)user_data);
}

/**
 * @brief Workspace load callback.
 * @param workspace Workspace (unused, accessed via user_data).
 * @param user_data Application instance.
 * @return `1` on success, `0` on failure.
 */
static int app_workspace_load(Workspace *workspace, void *user_data) {
  (void)workspace;
  return app_load_document((Application *)user_data);
}

static int app_workspace_execute_action(Workspace *workspace,
                                        WorkspaceActionType action,
                                        void *user_data) {
  Application *app = (Application *)user_data;
  (void)workspace;

  switch (action) {
  case WORKSPACE_ACTION_NEW_DOCUMENT:
    return app_new_document(app);
  case WORKSPACE_ACTION_OPEN_DOCUMENT:
    return app_open_document_with_picker(app);
  case WORKSPACE_ACTION_EXIT_APPLICATION:
    return app_exit_application(app);
  case WORKSPACE_ACTION_NONE:
  default:
    return 0;
  }
}

/**
 * @brief Open the startup document if it exists.
 * @param app Application instance.
 * @return No return value.
 */
static void app_open_startup_document(Application *app) {
  const char *path = app_current_document_path(app);

  if (!app) {
    return;
  }

  if (app_file_exists(path)) {
    if (!app_load_document(app)) {
      app_set_status(app, "Startup load failed: %s", path);
    }
    return;
  }

  app_set_status(app, "New empty document");
}

/**
 * @brief Build a tool context from application-owned workspace pointers.
 * @param app Application instance.
 * @return Tool context value.
 */
static ToolContext app_tool_context(Application *app) {
  return workspace_tool_context(app ? &app->workspace : NULL);
}

/**
 * @brief Decide whether pointer input should be blocked from canvas tools.
 * @return Non-zero when UI interaction or non-canvas regions should consume
 * input.
 *
 * Why:
 * - Prevents accidental drawing/selection while interacting with UI widgets.
 */
static int app_pointer_blocks_canvas(const Application *app, Vec2 screen_pos) {
  if (!app) {
    return 0;
  }

  if (app->ui && ui_system_blocks_pointer(app->ui, screen_pos)) {
    return 1;
  }

  return app->ui && !ui_system_point_in_canvas(app->ui, screen_pos);
}

/**
 * @brief Sync the tool controller pointer anchor with current cursor state.
 * @param app Application instance.
 * @return No return value.
 */
static void app_sync_tool_pointer_anchor(Application *app) {
  if (!app) {
    return;
  }

  tool_controller_set_pointer_anchor(
      &app->workspace.core.tools,
      app->cursor_screen,
      canvas_view_screen_to_world(&app->workspace.core.canvas, app->cursor_screen));
}

/**
 * @brief Refresh whether the cursor is inside the canvas area.
 * @param app Application instance.
 * @return No return value.
 */
static void app_sync_canvas_boundary(Application *app) {
  int inside_canvas = 0;

  if (!app || tool_controller_is_pointer_captured(&app->workspace.core.tools)) {
    return;
  }

  inside_canvas =
      app->ui ? ui_system_point_in_canvas(app->ui, app->cursor_screen) : 1;
  if (inside_canvas != app->cursor_inside_canvas) {
    app->cursor_inside_canvas = inside_canvas;
    app_sync_tool_pointer_anchor(app);
  }
}

/**
 * @brief Update the canvas viewport from the latest UI layout snapshot.
 * @param app Application instance.
 * @return No return value.
 */
static void update_canvas_viewport(Application *app) {
  RectF viewport;
  RectF fallback_window = {0.0f, 0.0f, 1.0f, 1.0f};

  if (!app) {
    return;
  }

  if (app->ui) {
    viewport = ui_system_content_bounds(app->ui);
    fallback_window = ui_system_window_bounds(app->ui);
    workspace_set_layout(&app->workspace, ui_system_layout(app->ui));
  } else {
    viewport = app->workspace.session.layout.canvas_content_bounds;
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
    app->workspace.core.canvas.background = ui_system_canvas_background(app->ui);
  }

  canvas_view_set_viewport(&app->workspace.core.canvas, viewport);
  app_sync_canvas_boundary(app);
}

/**
 * @brief Build a `ToolEvent` from the current cursor and previous pointer anchor.
 * @param app Application instance.
 * @param button Mouse button code.
 * @param mods Modifier key flags.
 * @param wheel_y Scroll delta.
 * @return Tool event value.
 */
static ToolEvent make_tool_event(Application *app, int button, int mods,
                                 float wheel_y) {
  ToolEvent event;
  Vec2 last_screen;
  Vec2 last_world;

  last_screen = tool_controller_last_screen(&app->workspace.core.tools);
  last_world = tool_controller_last_world(&app->workspace.core.tools);
  event.screen_pos = app->cursor_screen;
  event.world_pos =
      canvas_view_screen_to_world(&app->workspace.core.canvas, app->cursor_screen);
  event.delta_screen = vec2_sub(event.screen_pos, last_screen);
  event.delta_world = vec2_sub(event.world_pos, last_world);
  event.button = button;
  event.mods = mods;
  event.wheel_y = wheel_y;
  return event;
}

/**
 * @brief GLFW framebuffer resize callback.
 * @param handle GLFW window handle.
 * @param width New framebuffer width.
 * @param height New framebuffer height.
 * @return No return value.
 */
static void framebuffer_size_callback(GLFWwindow *handle, int width,
                                      int height) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  int window_width = 0;
  int window_height = 0;

  if (!app) {
    return;
  }

  glfwGetWindowSize(handle, &window_width, &window_height);
  app->window.width = window_width;
  app->window.height = window_height;
  app->window.framebuffer_width = width;
  app->window.framebuffer_height = height;
  update_canvas_viewport(app);
  render_system_resize(app->renderer, window_width, window_height, width, height);
}

/**
 * @brief GLFW cursor movement callback.
 * @param handle GLFW window handle.
 * @param xpos Cursor x position in screen coordinates.
 * @param ypos Cursor y position in screen coordinates.
 * @return No return value.
 */
static void cursor_pos_callback(GLFWwindow *handle, double xpos, double ypos) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  ToolContext context;
  ToolEvent event;

  if (!app) {
    return;
  }

  app->cursor_screen = vec2_make((float)xpos, (float)ypos);
  app_sync_canvas_boundary(app);

  if (!tool_controller_is_pointer_captured(&app->workspace.core.tools) &&
      app_pointer_blocks_canvas(app, app->cursor_screen)) {
    return;
  }

  context = app_tool_context(app);
  event = make_tool_event(app, -1, 0, 0.0f);

  tool_controller_pointer_move(&app->workspace.core.tools, &context, &event);
}

/**
 * @brief GLFW mouse button callback.
 * @param handle GLFW window handle.
 * @param button Mouse button code.
 * @param action Press/release action.
 * @param mods Modifier key flags.
 * @return No return value.
 */
static void mouse_button_callback(GLFWwindow *handle, int button, int action,
                                  int mods) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  ToolContext context;
  ToolEvent event;

  if (!app) {
    return;
  }

  if (action == GLFW_PRESS) {
    if (app->ui &&
        ui_system_handle_mouse_button(app->ui, app->cursor_screen, button,
                                      action)) {
      return;
    }
    if (!tool_controller_is_pointer_captured(&app->workspace.core.tools) &&
        app_pointer_blocks_canvas(app, app->cursor_screen)) {
      return;
    }
    context = app_tool_context(app);
    event = make_tool_event(app, button, mods, 0.0f);
    tool_controller_pointer_down(&app->workspace.core.tools, &context, &event);
  } else if (action == GLFW_RELEASE) {
    /* Ignore release outside canvas unless we previously captured the pointer.
       This keeps drag/end-state transitions consistent across window regions.
     */
    if (!tool_controller_is_pointer_captured(&app->workspace.core.tools)) {
      if (app_pointer_blocks_canvas(app, app->cursor_screen)) {
        return;
      }
      return;
    }
    context = app_tool_context(app);
    event = make_tool_event(app, button, mods, 0.0f);
    tool_controller_pointer_up(&app->workspace.core.tools, &context, &event);
  }
}

/**
 * @brief GLFW key callback for global shortcuts and active-tool keys.
 * @param handle [in] GLFW window handle.
 * @param key [in] Virtual key code.
 * @param scancode [in] Platform-specific scan code (unused).
 * @param action [in] Press/release/repeat.
 * @param mods [in] Modifier key flags.
 *
 * Why shortcut-first:
 * - Global document/view commands must win over tool-specific key handlers
 *   for predictable editor behavior.
 */
static void key_callback(GLFWwindow *handle, int key, int scancode, int action,
                         int mods) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  ToolContext context;
  InputRouterContext router_context;
  KeyEvent event;
  (void)scancode;

  if (!app) {
    return;
  }

  if (app->ui && ui_system_handle_key(app->ui, key, action)) {
    return;
  }

  context = app_tool_context(app);
  router_context.workspace = &app->workspace;
  router_context.tool_context = &context;
  router_context.ui_has_keyboard_focus =
      app->ui ? ui_system_has_active_interaction(app->ui) : 0;
  event.key = key;
  event.mods = mods;
  event.action = action;
  event.repeated = (action == GLFW_REPEAT);
  input_router_handle_key(&router_context, &event);
}

/**
 * @brief GLFW scroll callback.
 * @param handle GLFW window handle.
 * @param xoffset Horizontal scroll delta (unused).
 * @param yoffset Vertical scroll delta.
 * @return No return value.
 */
static void scroll_callback(GLFWwindow *handle, double xoffset,
                            double yoffset) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  ToolContext context;
  (void)xoffset;

  if (!app) {
    return;
  }

  if (!tool_controller_is_pointer_captured(&app->workspace.core.tools) &&
      app_pointer_blocks_canvas(app, app->cursor_screen)) {
    return;
  }

  context = app_tool_context(app);
  tool_controller_scroll(&app->workspace.core.tools, &context, app->cursor_screen,
                         (float)yoffset);
}

static void window_close_callback(GLFWwindow *handle) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);

  if (!app) {
    return;
  }

  glfwSetWindowShouldClose(handle, GLFW_FALSE);
  workspace_request_action(&app->workspace, WORKSPACE_ACTION_EXIT_APPLICATION);
}

/**
 * @brief Initialize all runtime subsystems in dependency order.
 * @return `0` on success, `-1` on failure.
 *
 * Risk note:
 * - This function has multiple allocation/init branches; each failure path must
 *   leave state safe for `app_shutdown()` best-effort cleanup.
 */
static int app_init(Application *app) {
  RectF viewport = {0.0f, 0.0f, 1440.0f, 900.0f};
  GLFWwindow *native_window = NULL;

  if (platform_window_init(&app->window, 1440, 900, "GLDraw Canvas") != 0) {
    LOG_ERROR("%s", "Failed to create window");
    return -1;
  }

  if (!workspace_init(&app->workspace, viewport, APP_KEYMAP_SETTINGS_PATH)) {
    LOG_ERROR("%s", "Failed to initialize workspace");
    return -1;
  }
  app_set_document_path(app, app_current_document_path(app));
  app->workspace.services.save_document = app_workspace_save;
  app->workspace.services.save_as_document = app_workspace_save_as;
  app->workspace.services.export_png = app_workspace_export_png;
  app->workspace.services.load_document = app_workspace_load;
  app->workspace.services.execute_action = app_workspace_execute_action;
  app->workspace.services.command_user_data = app;
  command_dispatcher_init(&app->dispatcher, &app->workspace);
  workspace_mark_saved(&app->workspace);
  workspace_set_status_message(&app->workspace, "Initializing editor");
  app->cursor_screen =
      vec2_make(app->window.width * 0.5f, app->window.height * 0.5f);
  app->cursor_inside_canvas = 1;

  {
    RenderDevice *device = render_device_factory_create_gl(&app->window);
    app->renderer = render_system_create(device, &app->window);
  }
  if (!app->renderer) {
    LOG_ERROR("%s", "Failed to initialize renderer");
    return -1;
  }

  app->ui = ui_system_create(&app->window);
  if (!app->ui) {
    LOG_ERROR("%s", "Failed to initialize UI");
    return -1;
  }
  {
    EditorActionSink sink;
    sink.callback = command_dispatcher_action_callback;
    sink.user_data = &app->dispatcher;
    ui_system_set_action_sink(app->ui, &sink);
  }

  native_window = platform_window_glfw_handle(&app->window);
  glfwSetWindowUserPointer(native_window, app);
  glfwSetFramebufferSizeCallback(native_window, framebuffer_size_callback);
  glfwSetCursorPosCallback(native_window, cursor_pos_callback);
  glfwSetMouseButtonCallback(native_window, mouse_button_callback);
  glfwSetKeyCallback(native_window, key_callback);
  glfwSetScrollCallback(native_window, scroll_callback);
  glfwSetWindowCloseCallback(native_window, window_close_callback);

  render_system_resize(app->renderer,
                       app->window.width,
                       app->window.height,
                       app->window.framebuffer_width,
                       app->window.framebuffer_height);
  update_canvas_viewport(app);
  app->cursor_inside_canvas =
      app->ui ? ui_system_point_in_canvas(app->ui, app->cursor_screen) : 1;
  app_sync_tool_pointer_anchor(app);
  app_open_startup_document(app);
  return 0;
}

/**
 * @brief Shutdown runtime subsystems in reverse ownership order.
 * @param app [in,out] Application state to tear down; safe no-op when `NULL`.
 * @return None.
 *
 * Risk note:
 * - Best-effort cleanup; destruction order mirrors `app_init()` to avoid
 *   dangling references to already-destroyed subsystems.
 */
static void app_shutdown(Application *app) {
  if (!app) {
    return;
  }

  ui_system_destroy(app->ui);
  render_system_destroy(app->renderer);
  editor_viewmodel_shutdown(&app->view_model);
  workspace_shutdown(&app->workspace);
  platform_window_shutdown(&app->window);
}

/**
 * @brief Application entry implementation called by `main`.
 * @return `0` on normal shutdown, `-1` on fatal startup failure.
 *
 * Time complexity:
 * - Main loop performs per-frame `O(object_count + ui_work)` operations.
 */
int app_run(void) {
  Application *app = (Application *)calloc(1, sizeof(*app));

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
    int framebuffer_w = 0;
    int framebuffer_h = 0;
    GLFWwindow *native_window = platform_window_glfw_handle(&app->window);
    ToolContext tool_context;

    platform_window_poll_events();
    glfwGetFramebufferSize(native_window, &framebuffer_w, &framebuffer_h);
    if (framebuffer_w <= 0 || framebuffer_h <= 0) {
      /* During monitor/fullscreen transitions some platforms briefly report
         zero-sized framebuffers. Avoid busy rendering loops in that state. */
      glfwWaitEventsTimeout(0.02);
      continue;
    }
    ui_system_begin_frame(app->ui);
    editor_viewmodel_build(&app->view_model, &app->workspace);
    ui_system_build(app->ui, &app->view_model);
    update_canvas_viewport(app);
    tool_context = workspace_tool_context(&app->workspace);
    render_system_draw(app->renderer,
                       &app->workspace.core.document,
                       &app->workspace.session.selection,
                       &app->workspace.core.canvas,
                       workspace_selection_preview_active(&app->workspace),
                       workspace_selection_preview_delta(&app->workspace),
                       tool_controller_overlay_object(&app->workspace.core.tools,
                                                      &tool_context));
    app_flush_pending_export_png(app);
    ui_system_render(app->ui);
    platform_window_swap_buffers(&app->window);
  }

  app_shutdown(app);
  free(app);
  return 0;
}
