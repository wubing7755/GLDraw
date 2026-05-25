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
#include <app/editor_controller.h>
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

#define APP_KEYMAP_SETTINGS_PATH "gldraw.keymap.json"

static int app_workspace_save(Workspace *workspace, void *user_data);
static int app_workspace_save_as(Workspace *workspace, void *user_data);
static int app_workspace_export_png(Workspace *workspace, void *user_data);
static int app_workspace_load(Workspace *workspace, void *user_data);

static int app_workspace_execute_action(Workspace *workspace,
                                        WorkspaceActionType action,
                                        void *user_data);

static int app_exit_application(Application *app) {
  if (!app) {
    return 0;
  }

  platform_window_set_should_close(&app->window, 1);
  workspace_set_status_message(app->workspace, "Closing application");
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
static void framebuffer_size_callback(PlatformWindow *window, int width,
                                      int height, void *user_data) {
  Application *app = (Application *)user_data;
  int window_width = 0;
  int window_height = 0;

  if (!app || !window) {
    return;
  }

  platform_window_get_size(window, &window_width, &window_height);
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
static void cursor_pos_callback(PlatformWindow *window,
                                double xpos,
                                double ypos,
                                void *user_data) {
  Application *app = (Application *)user_data;
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

/**
 * @brief GLFW mouse button callback.
 * @param handle GLFW window handle.
 * @param button Mouse button code.
 * @param action Press/release action.
 * @param mods Modifier key flags.
 * @return No return value.
 */
static void mouse_button_callback(PlatformWindow *window,
                                  int button,
                                  int action,
                                  int mods,
                                  void *user_data) {
  Application *app = (Application *)user_data;
  ToolContext context;
  ToolEvent event;
  (void)window;

  if (!app) {
    return;
  }

  if (action == GLFW_PRESS) {
    if (app->ui &&
        ui_system_handle_mouse_button(app->ui, app->cursor_screen, button,
                                      action)) {
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
    /* Ignore release outside canvas unless we previously captured the pointer.
       This keeps drag/end-state transitions consistent across window regions.
     */
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
static void key_callback(PlatformWindow *window,
                         int key,
                         int scancode,
                         int action,
                         int mods,
                         void *user_data) {
  Application *app = (Application *)user_data;
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

/**
 * @brief GLFW scroll callback.
 * @param handle GLFW window handle.
 * @param xoffset Horizontal scroll delta (unused).
 * @param yoffset Vertical scroll delta.
 * @return No return value.
 */
static void scroll_callback(PlatformWindow *window,
                            double xoffset,
                            double yoffset,
                            void *user_data) {
  Application *app = (Application *)user_data;
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
  editor_controller_scroll(app->workspace, &context, app->cursor_screen,
                           (float)yoffset);
}

static void window_close_callback(PlatformWindow *window, void *user_data) {
  Application *app = (Application *)user_data;

  if (!app || !window) {
    return;
  }

  platform_window_set_should_close(window, 0);
  workspace_request_action(app->workspace, WORKSPACE_ACTION_EXIT_APPLICATION);
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

  if (platform_window_init(&app->window, 1440, 900, "GLDraw Canvas") != 0) {
    LOG_ERROR("%s", "Failed to create window");
    return -1;
  }

  app->workspace = workspace_create(viewport, APP_KEYMAP_SETTINGS_PATH);
  if (!app->workspace) {
    LOG_ERROR("%s", "Failed to initialize workspace");
    return -1;
  }
  workspace_service_set_document_path(app->workspace,
                                      workspace_service_document_path(app->workspace));
  workspace_set_service_callbacks(app->workspace,
                                  app_workspace_save,
                                  app_workspace_save_as,
                                  app_workspace_export_png,
                                  app_workspace_load,
                                  app_workspace_execute_action,
                                  app);
  command_dispatcher_init(&app->dispatcher, app->workspace);
  workspace_mark_saved(app->workspace);
  workspace_set_status_message(app->workspace, "Initializing editor");
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

  platform_window_on_framebuffer_size(&app->window, framebuffer_size_callback, app);
  platform_window_on_cursor_pos(&app->window, cursor_pos_callback, app);
  platform_window_on_mouse_button(&app->window, mouse_button_callback, app);
  platform_window_on_key(&app->window, key_callback, app);
  platform_window_on_scroll(&app->window, scroll_callback, app);
  platform_window_on_close(&app->window, window_close_callback, app);

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
  workspace_destroy(app->workspace);
  app->workspace = NULL;
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
    EditorRenderScene scene;

    platform_window_poll_events();
    platform_window_get_framebuffer_size(&app->window, &framebuffer_w, &framebuffer_h);
    if (framebuffer_w <= 0 || framebuffer_h <= 0) {
      /* During monitor/fullscreen transitions some platforms briefly report
         zero-sized framebuffers. Avoid busy rendering loops in that state. */
      platform_window_wait_events_timeout(0.02);
      continue;
    }
    ui_system_begin_frame(app->ui);
    editor_viewmodel_build(&app->view_model, app->workspace);
    ui_system_build(app->ui, &app->view_model);
    application_runtime_update_canvas_viewport(app);
    if (editor_controller_render_scene(app->workspace, &scene)) {
      render_system_draw(app->renderer,
                         scene.document,
                         scene.selection,
                         scene.canvas,
                         scene.selection_preview_active,
                         scene.selection_preview_delta,
                         scene.overlay_object);
    }
    application_flush_pending_export_png(app);
    ui_system_render(app->ui);
    platform_window_swap_buffers(&app->window);
  }

  app_shutdown(app);
  free(app);
  return 0;
}
