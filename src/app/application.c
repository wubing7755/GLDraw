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
 * - Coordinates platform window, render system, UI system, tools, and
 * persistence.
 */
#include <app/application.h>

#include <app/workspace.h>
#include <base/log.h>
#include <base/math2d.h>
#include <document/persistence.h>
#include <platform/window.h>
#include <render/render_system.h>
#include <ui/ui_menu_actions.h>
#include <ui/ui_menu_def.h>
#include <ui/ui_system.h>

#include <GLFW/glfw3.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  PlatformWindow window;
  Workspace workspace;
  RenderSystem *renderer;
  UiSystem *ui;
  Vec2 cursor_screen;
  int cursor_inside_canvas;
} Application;

static int app_workspace_save(Workspace *workspace, void *user_data);
static int app_workspace_load(Workspace *workspace, void *user_data);

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

  if (!app || !fmt) {
    return;
  }

  va_start(args, fmt);
  vsnprintf(app->workspace.status_message,
            sizeof(app->workspace.status_message), fmt, args);
  va_end(args);
}

/** Check if file exists by opening it in read-binary mode. Complexity: `O(1)`
 * (metadata + open). */
static int app_file_exists(const char *path) {
  FILE *file = NULL;

  if (!path || path[0] == '\0') {
    return 0;
  }

  file = fopen(path, "rb");
  if (!file) {
    return 0;
  }

  fclose(file);
  return 1;
}

/** Return fallback document path used when workspace path is empty. Complexity:
 * `O(1)`. */
static const char *app_default_document_path(void) { return "document.json"; }

/** Resolve active document path (workspace path or default). Complexity:
 * `O(1)`. */
static const char *app_current_document_path(const Application *app) {
  if (app->workspace.current_document_path[0] != '\0') {
    return app->workspace.current_document_path;
  }
  return app_default_document_path();
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
  if (!app || !path) {
    return;
  }

  strncpy(app->workspace.current_document_path, path,
          sizeof(app->workspace.current_document_path) - 1u);
  app->workspace
      .current_document_path[sizeof(app->workspace.current_document_path) -
                             1u] = '\0';
}

/** Reset tool controller runtime state after major document changes.
 * Complexity: `O(tool_count)`. */
static void app_reset_tool_state(Application *app) {
  if (!app) {
    return;
  }

  tool_controller_shutdown(&app->workspace.tools);
  tool_controller_init(&app->workspace.tools);
}

/**
 * @brief Save current document to JSON and refresh dirty tracking.
 * @param app [in,out] Application state.
 * @return `1` on success, `0` on persistence failure.
 *
 * Edge cases:
 * - Fails if serializer or file I/O fails.
 *
 * Time complexity: dominated by serialization, roughly `O(object_count)`.
 */
static int app_save_document(Application *app) {
  const char *path = app_current_document_path(app);

  if (!document_save_json(&app->workspace.document, path)) {
    LOG_ERROR("%s", "Save document failed");
    app_set_status(app, "Save failed: %s", path);
    return 0;
  }

  app_set_document_path(app, path);
  workspace_mark_saved(&app->workspace);
  app_set_status(app, "Saved document: %s", path);
  LOG_INFO("Saved document: %s", path);
  return 1;
}

/**
 * @brief Load document from current path and reset dependent runtime state.
 * @param app [in,out] Application state.
 * @return `1` on success, `0` on missing file/parse/init failure.
 *
 * Why this structure:
 * - History and tool state are rebuilt after load so stale snapshots/pointers
 *   cannot reference objects from the previous document.
 *
 * Time complexity: dominated by parse/build, roughly `O(file_size +
 * object_count)`.
 */
static int app_load_document(Application *app) {
  const char *path = app_current_document_path(app);

  if (!app_file_exists(path)) {
    LOG_WARN("Document file not found: %s", path);
    app_set_status(app, "Document not found: %s", path);
    return 0;
  }

  if (!document_load_json(&app->workspace.document, path)) {
    LOG_ERROR("%s", "Load document failed");
    app_set_status(app, "Load failed: %s", path);
    return 0;
  }

  /* Rebuild history against the newly loaded object graph.
     Reusing old snapshots would leave dangling object pointers. */
  document_history_shutdown(&app->workspace.history);
  if (!document_history_init(&app->workspace.history)) {
    LOG_ERROR("%s", "Failed to reinitialize history after document load");
    app_set_status(app, "History reset failed after load");
    return 0;
  }
  app_reset_tool_state(app);
  app_set_document_path(app, path);
  workspace_mark_saved(&app->workspace);
  app_set_status(app, "Loaded document: %s", path);
  LOG_INFO("Loaded document: %s", path);
  return 1;
}

/** Workspace save callback adapter. Complexity: same as `app_save_document`. */
static int app_workspace_save(Workspace *workspace, void *user_data) {
  (void)workspace;
  return app_save_document((Application *)user_data);
}

/** Workspace load callback adapter. Complexity: same as `app_load_document`. */
static int app_workspace_load(Workspace *workspace, void *user_data) {
  (void)workspace;
  return app_load_document((Application *)user_data);
}

/** Load startup document if present; otherwise keep empty document. Complexity:
 * `O(file_size)` when file exists. */
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

/** Build tool context struct from application-owned workspace pointers.
 * Complexity: `O(1)`. */
static ToolContext app_tool_context(Application *app) {
  ToolContext context;
  context.workspace = &app->workspace;
  context.document = &app->workspace.document;
  context.history = &app->workspace.history;
  context.canvas = &app->workspace.canvas;
  return context;
}

/**
 * @brief Decide whether pointer input should be blocked from canvas tools.
 * @return Non-zero when UI interaction or non-canvas regions should consume
 * input.
 *
 * Why:
 * - Prevents accidental drawing/selection while interacting with UI widgets.
 *
 * Complexity: `O(1)`.
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

/** Sync tool controller's last pointer anchor with current cursor state.
 * Complexity: `O(1)`. */
static void app_sync_tool_pointer_anchor(Application *app) {
  if (!app) {
    return;
  }

  app->workspace.tools.last_screen = app->cursor_screen;
  app->workspace.tools.last_world =
      canvas_view_screen_to_world(&app->workspace.canvas, app->cursor_screen);
}

/** Refresh cursor-in-canvas flag for uncaptured pointer movement. Complexity:
 * `O(1)`. */
static void app_sync_canvas_boundary(Application *app) {
  int inside_canvas = 0;

  if (!app || app->workspace.tools.pointer_captured) {
    return;
  }

  inside_canvas =
      app->ui ? ui_system_point_in_canvas(app->ui, app->cursor_screen) : 1;
  if (inside_canvas != app->cursor_inside_canvas) {
    app->cursor_inside_canvas = inside_canvas;
    app_sync_tool_pointer_anchor(app);
  }
}

/** Update canvas viewport and theme background from latest UI layout.
 * Complexity: `O(1)`. */
static void update_canvas_viewport(Application *app) {
  RectF viewport;
  RectF fallback_window = {0.0f, 0.0f, 1.0f, 1.0f};

  if (!app) {
    return;
  }

  if (app->ui) {
    viewport = ui_system_content_bounds(app->ui);
    fallback_window = ui_system_window_bounds(app->ui);
  } else {
    viewport = app->workspace.layout.canvas_content_bounds;
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
    app->workspace.canvas.background = ui_system_canvas_background(app->ui);
  }

  canvas_view_set_viewport(&app->workspace.canvas, viewport);
  app_sync_canvas_boundary(app);
}

/** Build a `ToolEvent` from current cursor and previous anchor state.
 * Complexity: `O(1)`. */
static ToolEvent make_tool_event(Application *app, int button, int mods,
                                 float wheel_y) {
  ToolEvent event;
  event.screen_pos = app->cursor_screen;
  event.world_pos =
      canvas_view_screen_to_world(&app->workspace.canvas, app->cursor_screen);
  event.delta_screen =
      vec2_sub(event.screen_pos, app->workspace.tools.last_screen);
  event.delta_world =
      vec2_sub(event.world_pos, app->workspace.tools.last_world);
  event.button = button;
  event.mods = mods;
  event.wheel_y = wheel_y;
  return event;
}

/** GLFW framebuffer resize callback: update viewport and renderer dimensions.
 */
static void framebuffer_size_callback(GLFWwindow *handle, int width,
                                      int height) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  if (!app) {
    return;
  }
  app->window.width = width;
  app->window.height = height;
  update_canvas_viewport(app);
  render_system_resize(app->renderer, width, height);
}

/** GLFW cursor callback: route pointer-move to active tool when canvas is
 * interactive. */
static void cursor_pos_callback(GLFWwindow *handle, double xpos, double ypos) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  ToolContext context;
  ToolEvent event;

  if (!app) {
    return;
  }

  app->cursor_screen = vec2_make((float)xpos, (float)ypos);
  app_sync_canvas_boundary(app);

  if (!app->workspace.tools.pointer_captured &&
      app_pointer_blocks_canvas(app, app->cursor_screen)) {
    return;
  }

  context = app_tool_context(app);
  event = make_tool_event(app, -1, 0, 0.0f);

  tool_controller_pointer_move(&app->workspace.tools, &context, &event);
}

/** GLFW mouse callback: route press/release with pointer capture semantics. */
static void mouse_button_callback(GLFWwindow *handle, int button, int action,
                                  int mods) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  ToolContext context;
  ToolEvent event;

  if (!app) {
    return;
  }

  if (action == GLFW_PRESS) {
    if (!app->workspace.tools.pointer_captured &&
        app_pointer_blocks_canvas(app, app->cursor_screen)) {
      return;
    }
    context = app_tool_context(app);
    event = make_tool_event(app, button, mods, 0.0f);
    tool_controller_pointer_down(&app->workspace.tools, &context, &event);
  } else if (action == GLFW_RELEASE) {
    /* Ignore release outside canvas unless we previously captured the pointer.
       This keeps drag/end-state transitions consistent across window regions.
     */
    if (!app->workspace.tools.pointer_captured) {
      if (app_pointer_blocks_canvas(app, app->cursor_screen)) {
        return;
      }
      return;
    }
    context = app_tool_context(app);
    event = make_tool_event(app, button, mods, 0.0f);
    tool_controller_pointer_up(&app->workspace.tools, &context, &event);
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
  (void)scancode;

  if (!app || action != GLFW_PRESS) {
    return;
  }

  if (key == GLFW_KEY_ESCAPE &&
      app->workspace.tools.active_kind == TOOL_KIND_SELECT) {
    glfwSetWindowShouldClose(handle, GLFW_TRUE);
    return;
  }

  /* Menu shortcuts via centralized handler */
  if ((mods & GLFW_MOD_CONTROL) != 0) {
    switch (key) {
    case GLFW_KEY_N:
      ui_menu_execute(&app->workspace, MENU_ID_FILE_NEW);
      return;
    case GLFW_KEY_O:
      app_load_document(app);
      return;
    case GLFW_KEY_S:
      app_save_document(app);
      return;
    case GLFW_KEY_Z:
      ui_menu_execute(&app->workspace, MENU_ID_EDIT_UNDO);
      return;
    case GLFW_KEY_Y:
      ui_menu_execute(&app->workspace, MENU_ID_EDIT_REDO);
      return;
    case GLFW_KEY_A:
      ui_menu_execute(&app->workspace, MENU_ID_EDIT_SELECT_ALL);
      return;
    case GLFW_KEY_0:
      ui_menu_execute(&app->workspace, MENU_ID_VIEW_ZOOM_FIT);
      return;
    case GLFW_KEY_EQUAL:
    case GLFW_KEY_KP_ADD:
      ui_menu_execute(&app->workspace, MENU_ID_VIEW_ZOOM_IN);
      return;
    case GLFW_KEY_MINUS:
    case GLFW_KEY_KP_SUBTRACT:
      ui_menu_execute(&app->workspace, MENU_ID_VIEW_ZOOM_OUT);
      return;
    default:
      break;
    }
  }

  /* Help shortcut (question mark) */
  if (key == GLFW_KEY_SLASH && (mods & GLFW_MOD_SHIFT) != 0) {
    ui_menu_execute(&app->workspace, MENU_ID_HELP_SHORTCUTS);
    return;
  }

  context = app_tool_context(app);
  tool_controller_key_down(&app->workspace.tools, &context, key, mods);
}

/** GLFW scroll callback: zoom canvas at cursor unless blocked by UI. */
static void scroll_callback(GLFWwindow *handle, double xoffset,
                            double yoffset) {
  Application *app = (Application *)glfwGetWindowUserPointer(handle);
  ToolContext context;
  (void)xoffset;

  if (!app) {
    return;
  }

  if (!app->workspace.tools.pointer_captured &&
      app_pointer_blocks_canvas(app, app->cursor_screen)) {
    return;
  }

  context = app_tool_context(app);
  tool_controller_scroll(&app->workspace.tools, &context, app->cursor_screen,
                         (float)yoffset);
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

  document_init(&app->workspace.document);
  if (!document_history_init(&app->workspace.history)) {
    LOG_ERROR("%s", "Failed to initialize document history");
    return -1;
  }
  canvas_view_init(&app->workspace.canvas, &app->workspace.document, viewport);
  tool_controller_init(&app->workspace.tools);
  app_set_document_path(app, app_default_document_path());
  app->workspace.save_document = app_workspace_save;
  app->workspace.load_document = app_workspace_load;
  app->workspace.command_user_data = app;
  workspace_mark_saved(&app->workspace);
  app_set_status(app, "Initializing editor");
  app->cursor_screen =
      vec2_make(app->window.width * 0.5f, app->window.height * 0.5f);
  app->cursor_inside_canvas = 1;

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
  tool_controller_shutdown(&app->workspace.tools);
  document_history_shutdown(&app->workspace.history);
  document_shutdown(&app->workspace.document);
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

    platform_window_poll_events();
    glfwGetFramebufferSize(app->window.handle, &framebuffer_w, &framebuffer_h);
    if (framebuffer_w <= 0 || framebuffer_h <= 0) {
      /* During monitor/fullscreen transitions some platforms briefly report
         zero-sized framebuffers. Avoid busy rendering loops in that state. */
      glfwWaitEventsTimeout(0.02);
      continue;
    }
    ui_system_begin_frame(app->ui);
    ui_system_build(app->ui, &app->workspace);
    update_canvas_viewport(app);
    render_system_draw(app->renderer, &app->workspace.document,
                       &app->workspace.canvas,
                       tool_controller_overlay_object(&app->workspace.tools));
    ui_system_render(app->ui);
    platform_window_swap_buffers(&app->window);
  }

  app_shutdown(app);
  free(app);
  return 0;
}
