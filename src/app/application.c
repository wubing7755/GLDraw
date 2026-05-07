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

#include <app/application_file_actions.h>
#include <app/application_runtime.h>
#include <app/command_registry.h>
#include <app/command_dispatcher.h>
#include <app/workspace_actions.h>
#include <app/workspace.h>
#include <app/workspace_service.h>
#include <base/log.h>
#include <base/math2d.h>
#include <input/input_router.h>
#include <render/render_device_factory.h>

#include <GLFW/glfw3.h>

#include <stdlib.h>

#include "application_internal.h"
#include "platform/window_internal.h"

#define APP_KEYMAP_SETTINGS_PATH "gldraw.keymap.json"

static int app_workspace_save(Workspace *workspace, void *user_data);
static int app_workspace_save_as(Workspace *workspace, void *user_data);
static int app_workspace_export_png(Workspace *workspace, void *user_data);
static int app_workspace_load(Workspace *workspace, void *user_data);

static int app_workspace_execute_action(Workspace *workspace,
                                        WorkspaceActionType action,
                                        void *user_data);

static int app_exit_application(Application *app) {
  GLFWwindow *handle = app ? platform_window_glfw_handle(&app->window) : NULL;

  if (!app || !handle) {
    return 0;
  }

  glfwSetWindowShouldClose(handle, GLFW_TRUE);
  workspace_set_status_message(&app->workspace, "Closing application");
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
  return application_save_document((Application *)user_data);
}

static int app_workspace_save_as(Workspace *workspace, void *user_data) {
  (void)workspace;
  return application_save_as_document((Application *)user_data);
}

static int app_workspace_export_png(Workspace *workspace, void *user_data) {
  (void)workspace;
  return application_request_export_png((Application *)user_data);
}

/**
 * @brief Workspace load callback.
 * @param workspace Workspace (unused, accessed via user_data).
 * @param user_data Application instance.
 * @return `1` on success, `0` on failure.
 */
static int app_workspace_load(Workspace *workspace, void *user_data) {
  (void)workspace;
  return application_load_document((Application *)user_data);
}

static int app_workspace_execute_action(Workspace *workspace,
                                        WorkspaceActionType action,
                                        void *user_data) {
  Application *app = (Application *)user_data;
  (void)workspace;

  switch (action) {
  case WORKSPACE_ACTION_NEW_DOCUMENT:
    return application_new_document(app);
  case WORKSPACE_ACTION_OPEN_DOCUMENT:
    return application_open_document_with_picker(app);
  case WORKSPACE_ACTION_EXIT_APPLICATION:
    return app_exit_application(app);
  case WORKSPACE_ACTION_NONE:
  default:
    return 0;
  }
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
  application_runtime_update_canvas_viewport(app);
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
  application_runtime_sync_canvas_boundary(app);

  if (!tool_controller_is_pointer_captured(&app->workspace.core.tools) &&
      application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
    return;
  }

  context = application_runtime_tool_context(app);
  event = application_runtime_make_tool_event(app, -1, 0, 0.0f);

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
        application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
      return;
    }
    context = application_runtime_tool_context(app);
    event = application_runtime_make_tool_event(app, button, mods, 0.0f);
    tool_controller_pointer_down(&app->workspace.core.tools, &context, &event);
  } else if (action == GLFW_RELEASE) {
    /* Ignore release outside canvas unless we previously captured the pointer.
       This keeps drag/end-state transitions consistent across window regions.
     */
    if (!tool_controller_is_pointer_captured(&app->workspace.core.tools)) {
      if (application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
        return;
      }
      return;
    }
    context = application_runtime_tool_context(app);
    event = application_runtime_make_tool_event(app, button, mods, 0.0f);
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

  context = application_runtime_tool_context(app);
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
      application_runtime_pointer_blocks_canvas(app, app->cursor_screen)) {
    return;
  }

  context = application_runtime_tool_context(app);
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
  workspace_service_set_document_path(&app->workspace,
                                      workspace_service_document_path(&app->workspace));
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
  application_runtime_update_canvas_viewport(app);
  app->cursor_inside_canvas =
      app->ui ? ui_system_point_in_canvas(app->ui, app->cursor_screen) : 1;
  application_runtime_sync_tool_pointer_anchor(app);
  application_open_startup_document(app);
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
    application_runtime_update_canvas_viewport(app);
    tool_context = application_runtime_tool_context(app);
    render_system_draw(app->renderer,
                       &app->workspace.core.document,
                       &app->workspace.session.selection,
                       &app->workspace.core.canvas,
                       workspace_selection_preview_active(&app->workspace),
                       workspace_selection_preview_delta(&app->workspace),
                       tool_controller_overlay_object(&app->workspace.core.tools,
                                                      &tool_context));
    application_flush_pending_export_png(app);
    ui_system_render(app->ui);
    platform_window_swap_buffers(&app->window);
  }

  app_shutdown(app);
  free(app);
  return 0;
}
