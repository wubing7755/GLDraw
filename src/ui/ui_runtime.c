/**
 * @file ui_runtime.c
 * @brief UiSystem lifecycle, frame build, action bridge, and layout access runtime.
 */
#include <ui/ui_system.h>

#include <base/math2d.h>
#include <glad/glad.h>

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

  ui->ctx = platform_window_nuklear_init(&ui->glfw, window);
  if (!ui->ctx) {
    free(ui);
    return NULL;
  }

  platform_window_nuklear_font_stash_begin(&ui->glfw, &atlas);
  platform_window_nuklear_font_stash_end(&ui->glfw);

  snprintf(ui->theme_settings_path, sizeof(ui->theme_settings_path), "%s",
           UI_THEME_SETTINGS_PATH);

  ui_system_reload_themes(ui, 0, UI_THEME_RELOAD_REASON_STARTUP);
  ui->theme_watch_last_check_seconds = platform_time_seconds();

  ui->menu_bar = ui_menubar_create(ui->ctx);
  if (ui->menu_bar) {
    ui_system_sync_menubar_themes(ui);
  }
  ui_system_load_theme_from_settings(ui);

  ui->inspector_anim_t = 1.0f;
  ui->inspector_target_visible = 1;
  ui->inspector_anim_initialized = 0;
  ui->last_frame_seconds = platform_time_seconds();
  ui->window = window;

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
  platform_window_nuklear_shutdown(&ui->glfw);
  free(ui);
}

void ui_system_begin_frame(UiSystem *ui) {
  if (ui) {
    platform_window_nuklear_new_frame(&ui->glfw);
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
  platform_window_nuklear_render(&ui->glfw,
                                 NK_ANTI_ALIASING_ON,
                                 UI_MAX_VERTEX_BUFFER,
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
