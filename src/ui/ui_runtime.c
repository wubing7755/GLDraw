/**
 * @file ui_runtime.c
 * @brief UiSystem lifecycle, frame build, action bridge, and layout access runtime.
 */
#include <ui/ui_system.h>

#include <base/math2d.h>
#include <glad/glad.h>
#include "platform/window_internal.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION

#include "nuklear/nuklear.h"
#include "nuklear/nuklear_glfw_gl3.h"
#include "ui/ui_context_menu.h"
#include "ui/ui_system_internal.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define UI_MAX_VERTEX_BUFFER (512 * 1024)
#define UI_MAX_ELEMENT_BUFFER (128 * 1024)
#define UI_MIN_CANVAS_WIDTH 320.0f

void ui_system_emit_action(UiSystem *ui, const EditorAction *action) {
  if (!ui || !action) {
    return;
  }

  editor_action_emit(&ui->action_sink, action);
}

void ui_system_emit_status(UiSystem *ui, const char *fmt, ...) {
  char buffer[EDITOR_ACTION_STATUS_CAPACITY];
  va_list args;
  EditorAction action;

  if (!ui || !fmt) {
    return;
  }

  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  action = editor_action_make_set_status_message(buffer);
  ui_system_emit_action(ui, &action);
}

UiSystem *ui_system_create(PlatformWindow *window) {
  UiSystem *ui = (UiSystem *)calloc(1, sizeof(*ui));
  struct nk_font_atlas *atlas = NULL;

  if (!ui) {
    return NULL;
  }

  ui->ctx = nk_glfw3_init(&ui->glfw, platform_window_glfw_handle(window), 0);
  if (!ui->ctx) {
    free(ui);
    return NULL;
  }

  nk_glfw3_font_stash_begin(&ui->glfw, &atlas);
  nk_glfw3_font_stash_end(&ui->glfw);

  snprintf(ui->theme_settings_path, sizeof(ui->theme_settings_path), "%s",
           UI_THEME_SETTINGS_PATH);

  ui_system_reload_themes(ui, 0, UI_THEME_RELOAD_REASON_STARTUP);
  ui->theme_watch_last_check_seconds = glfwGetTime();

  ui->menu_bar = ui_menubar_create(ui->ctx);
  if (ui->menu_bar) {
    ui_system_sync_menubar_themes(ui);
  }
  ui_system_load_theme_from_settings(ui);

  ui->inspector_anim_t = 1.0f;
  ui->inspector_target_visible = 1;
  ui->inspector_anim_initialized = 0;
  ui->last_frame_seconds = glfwGetTime();
  ui->window_handle = platform_window_glfw_handle(window);

  return ui;
}

void ui_system_destroy(UiSystem *ui) {
  if (!ui) {
    return;
  }
  if (ui->menu_bar) {
    ui_menubar_destroy(ui->menu_bar);
    ui->menu_bar = NULL;
  }
  nk_glfw3_shutdown(&ui->glfw);
  free(ui);
}

void ui_system_begin_frame(UiSystem *ui) {
  if (ui) {
    nk_glfw3_new_frame(&ui->glfw);
  }
}

void ui_system_set_action_sink(UiSystem *ui, const EditorActionSink *sink) {
  if (!ui) {
    return;
  }

  if (sink) {
    ui->action_sink = *sink;
  } else {
    memset(&ui->action_sink, 0, sizeof(ui->action_sink));
  }
}

void ui_system_build(UiSystem *ui, const EditorViewModel *view_model) {
  int width = 0;
  int height = 0;
  float content_top = 0.0f;
  float content_bottom = 0.0f;
  float content_height = 0.0f;
  float needed_width = 0.0f;
  int inspector_requested = 0;
  int inspector_target_visible = 0;
  int inspector_render_visible = 0;
  RectF rail_bounds = {0};
  RectF inspector_bounds = {0};
  double now_seconds;
  float dt_seconds;
  float anim_step;
  float inspector_eased_t;
  float inspector_hidden_x;
  float inspector_shown_x;

  if (!ui || !view_model) {
    return;
  }

  ui->last_selection_count = view_model->summary.selection_count;
  ui->modal_active = view_model->active_dialog.kind != UI_DIALOG_NONE &&
                     view_model->active_dialog.modal;
  if (ui->modal_active) {
    ui_context_menu_reset(ui);
  } else {
    ui->dialog_kind_snapshot = UI_DIALOG_NONE;
  }

  if (ui->window_handle) {
    glfwGetWindowSize(ui->window_handle, &width, &height);
  }
  if (width <= 0 || height <= 0) {
    GLFWwindow *current_context = glfwGetCurrentContext();
    if (current_context) {
      glfwGetWindowSize(current_context, &width, &height);
    }
  }

  if (width <= 0 || height <= 0) {
    ui_publish_layout(ui, 1, 1);
    return;
  }

  now_seconds = glfwGetTime();
  dt_seconds = (float)(now_seconds - ui->last_frame_seconds);
  ui->last_frame_seconds = now_seconds;
  dt_seconds = ui_clampf(dt_seconds, 0.0f, 0.10f);
  ui_system_poll_theme_hot_reload(ui, now_seconds);

  if (ui->menu_bar && !ui->modal_active) {
    int requested_theme_index = -1;
    int requested_theme_reload = 0;
    const UiThemeDescriptor *requested_theme = NULL;

    ui_menubar_set_height(ui->menu_bar, ui->theme.menu_height);
    ui_menubar_build(ui->menu_bar, view_model, &ui->action_sink, width);

    requested_theme_reload = ui_menubar_take_theme_reload_request(ui->menu_bar);
    if (requested_theme_reload) {
      ui_system_reload_themes(ui, 1, UI_THEME_RELOAD_REASON_MANUAL);
    }

    requested_theme_index = ui_menubar_take_theme_request(ui->menu_bar);
    if (requested_theme_index >= 0) {
      requested_theme = ui_theme_descriptor_at(requested_theme_index);
      if (requested_theme && ui_system_set_theme(ui, requested_theme->id, 1)) {
        ui_system_emit_status(ui, "Theme: %s",
                              requested_theme->label ? requested_theme->label
                                                     : requested_theme->id);
      }
    }
  }

  ui->appbar_bounds.x = 0.0f;
  ui->appbar_bounds.y = 0.0f;
  ui->appbar_bounds.w = (float)width;
  ui->appbar_bounds.h = ui_menubar_height(ui->menu_bar);

  content_top = ui->appbar_bounds.h;
  content_bottom = (float)height - ui->theme.status_height;
  content_height = content_bottom - content_top;
  if (content_height < 1.0f) {
    content_height = 1.0f;
  }

  rail_bounds.x = 0.0f;
  rail_bounds.y = content_top;
  rail_bounds.w = ui->theme.tool_rail_width;
  rail_bounds.h = content_height;
  ui->rail_bounds = rail_bounds;
  ui_tool_rail(ui, view_model, rail_bounds);

  inspector_requested = ui_menubar_inspector_visible(ui->menu_bar);
  needed_width = ui->theme.tool_rail_width + UI_MIN_CANVAS_WIDTH;
  if (inspector_requested) {
    needed_width += ui->theme.panel_width;
  }
  inspector_target_visible =
      inspector_requested && ((float)width >= needed_width);
  ui->inspector_target_visible = inspector_target_visible;

  if (!ui->inspector_anim_initialized) {
    ui->inspector_anim_t = inspector_target_visible ? 1.0f : 0.0f;
    ui->inspector_anim_initialized = 1;
  } else if (!ui->theme.enable_transitions ||
             ui->theme.transition_duration <= 1e-4f) {
    ui->inspector_anim_t = inspector_target_visible ? 1.0f : 0.0f;
  } else {
    anim_step = dt_seconds / ui->theme.transition_duration;
    if (ui->inspector_target_visible) {
      ui->inspector_anim_t =
          ui_clampf(ui->inspector_anim_t + anim_step, 0.0f, 1.0f);
    } else {
      ui->inspector_anim_t =
          ui_clampf(ui->inspector_anim_t - anim_step, 0.0f, 1.0f);
    }
  }

  inspector_render_visible = (ui->inspector_anim_t > 1e-3f);
  if (inspector_render_visible) {
    inspector_eased_t = ui_smoothstep(ui->inspector_anim_t);
    inspector_bounds.w = ui->theme.panel_width;
    inspector_bounds.h = content_height;
    inspector_hidden_x = (float)width;
    inspector_shown_x = (float)width - inspector_bounds.w;
    inspector_bounds.x =
        inspector_hidden_x +
        (inspector_shown_x - inspector_hidden_x) * inspector_eased_t;
    inspector_bounds.y = content_top;
    ui->panel_bounds = inspector_bounds;
    ui_selection_panel(ui, view_model, inspector_bounds);
  } else {
    ui->panel_bounds.x = 0.0f;
    ui->panel_bounds.y = 0.0f;
    ui->panel_bounds.w = 0.0f;
    ui->panel_bounds.h = 0.0f;
  }

  ui_status_bar(ui, view_model, width, height);
  ui_modal_dialogs(ui, view_model, width, height);

  ui->content_bounds.x = ui->rail_bounds.x + ui->rail_bounds.w;
  ui->content_bounds.y = ui->appbar_bounds.y + ui->appbar_bounds.h;
  if (ui->panel_bounds.w > 1e-3f) {
    ui->content_bounds.w = ui->panel_bounds.x - ui->content_bounds.x;
  } else {
    ui->content_bounds.w = (float)width - ui->content_bounds.x;
  }
  ui->content_bounds.h = ui->status_bounds.y - ui->content_bounds.y;

  if (ui->content_bounds.w < 1.0f) {
    ui->content_bounds.w = 1.0f;
  }
  if (ui->content_bounds.h < 1.0f) {
    ui->content_bounds.h = 1.0f;
  }

  if (!ui->modal_active) {
    ui_context_menu_build(ui, view_model, &ui->action_sink);
  }

  ui_publish_layout(ui, width, height);
}

int ui_system_handle_key(UiSystem *ui, int key, int action) {
  return ui_context_menu_handle_key(ui, key, action);
}

int ui_system_handle_mouse_button(UiSystem *ui, Vec2 screen_pos, int button,
                                  int action) {
  return ui_context_menu_handle_mouse_button(ui, screen_pos, button, action);
}

void ui_system_render(UiSystem *ui) {
  if (!ui) {
    return;
  }
  nk_glfw3_render(&ui->glfw, NK_ANTI_ALIASING_ON, UI_MAX_VERTEX_BUFFER,
                  UI_MAX_ELEMENT_BUFFER);
}

int ui_system_has_active_interaction(const UiSystem *ui) {
  if (!ui || !ui->ctx) {
    return 0;
  }

  return ui_context_menu_is_open(ui) || ui_menubar_menu_open(ui->menu_bar) ||
         nk_item_is_any_active(ui->ctx);
}

int ui_system_blocks_pointer(const UiSystem *ui, Vec2 screen_pos) {
  if (!ui || !ui->ctx) {
    return 0;
  }

  if (ui->modal_active) {
    (void)screen_pos;
    return 1;
  }

  if (ui_system_has_active_interaction(ui)) {
    return 1;
  }

  return rectf_contains_point(&ui->layout_snapshot.appbar_bounds, screen_pos) ||
         rectf_contains_point(&ui->layout_snapshot.rail_bounds, screen_pos) ||
         rectf_contains_point(&ui->layout_snapshot.panel_bounds, screen_pos) ||
         rectf_contains_point(&ui->layout_snapshot.status_bounds, screen_pos);
}

RectF ui_system_window_bounds(const UiSystem *ui) {
  RectF bounds = {0.0f, 0.0f, 1.0f, 1.0f};

  if (!ui) {
    return bounds;
  }

  bounds = ui->layout_snapshot.window_bounds;
  if (bounds.w < 1.0f) {
    bounds.w = 1.0f;
  }
  if (bounds.h < 1.0f) {
    bounds.h = 1.0f;
  }
  return bounds;
}

RectF ui_system_content_bounds(const UiSystem *ui) {
  RectF bounds = {0.0f, 0.0f, 0.0f, 0.0f};

  if (!ui) {
    return bounds;
  }

  return ui->layout_snapshot.canvas_content_bounds;
}

int ui_system_point_in_canvas(const UiSystem *ui, Vec2 screen_pos) {
  RectF canvas_bounds = ui_system_content_bounds(ui);
  RectF window_bounds = ui_system_window_bounds(ui);

  if (canvas_bounds.w <= 1.0f || canvas_bounds.h <= 1.0f) {
    canvas_bounds = window_bounds;
  }

  return rectf_contains_point(&canvas_bounds, screen_pos);
}

Color ui_system_canvas_background(const UiSystem *ui) {
  Color background = {0.11f, 0.12f, 0.14f, 1.0f};

  if (!ui) {
    return background;
  }

  background.r = (float)ui->theme.canvas_background.r / 255.0f;
  background.g = (float)ui->theme.canvas_background.g / 255.0f;
  background.b = (float)ui->theme.canvas_background.b / 255.0f;
  background.a = (float)ui->theme.canvas_background.a / 255.0f;
  return background;
}

WorkspaceLayout ui_system_layout(const UiSystem *ui) {
  WorkspaceLayout layout;

  memset(&layout, 0, sizeof(layout));
  if (!ui) {
    return layout;
  }

  return ui->layout_snapshot;
}
