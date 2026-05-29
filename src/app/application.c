/**
 * @file application.c
 * @brief Application lifecycle and frame loop orchestration.
 *
 * Role in project:
 * - Owns application lifetime from initialization to shutdown.
 * - Sequences subsystem setup, per-frame work, and teardown.
 *
 * Module relationships:
 * - Delegates platform callbacks and workspace service callbacks to focused modules.
 * - Coordinates platform window, render system, UI system, and workspace runtime state.
 */
#include <app/application.h>

#include <app/application_file_actions.h>
#include <app/application_runtime.h>
#include <app/command_dispatcher.h>
#include <app/editor_controller.h>
#include <app/workspace.h>
#include <app/workspace_service.h>
#include <base/log.h>
#include <base/math2d.h>
#include <render/render_device_factory.h>

#include <stdlib.h>

#include "application_internal.h"

#define APP_KEYMAP_SETTINGS_PATH "gldraw.keymap.json"

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
  application_register_workspace_services(app);
  command_dispatcher_init(&app->dispatcher, app->workspace);
  workspace_mark_saved(app->workspace);
  workspace_set_status_message(app->workspace, "Initializing editor");
  app->cursor_screen =
      vec2_make(app->window.width * 0.5f, app->window.height * 0.5f);
  app->cursor_inside_canvas = 1;

  {
    RenderDevice *device = render_device_factory_create_gl(&app->window);
    app->renderer = render_system_create(device, &app->window);
    if (!app->renderer) {
      render_device_destroy(device);
    }
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

  application_register_platform_callbacks(app);

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
    RenderSceneDesc render_scene;

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
      render_scene.document = scene.document;
      render_scene.selection = scene.selection;
      render_scene.canvas = scene.canvas;
      render_scene.selection_preview_active = scene.selection_preview_active;
      render_scene.selection_preview_delta = scene.selection_preview_delta;
      render_scene.overlay_object = scene.overlay_object;
      render_system_draw(app->renderer, &render_scene);
    }
    application_flush_pending_export_png(app);
    ui_system_render(app->ui);
    platform_window_swap_buffers(&app->window);
  }

  app_shutdown(app);
  free(app);
  return 0;
}
