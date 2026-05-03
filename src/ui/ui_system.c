/**
 * @file ui_system.c
 * @brief Nuklear UI system implementation (menu, tool rail, inspector, status).
 */
#include <ui/ui_system.h>
#include <ui/ui_menubar.h>
#include <ui/ui_dialog.h>

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
#include <ui/ui_theme.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UI_MAX_VERTEX_BUFFER (512 * 1024)
#define UI_MAX_ELEMENT_BUFFER (128 * 1024)
#define UI_MIN_CANVAS_WIDTH 320.0f
#define UI_THEME_SETTINGS_PATH "gldraw.settings.json"
#define UI_THEME_DIRECTORY_PATH "themes"
#define UI_THEME_WATCH_INTERVAL_SECONDS 0.35

typedef enum UiThemeReloadReason {
  UI_THEME_RELOAD_REASON_STARTUP = 0,
  UI_THEME_RELOAD_REASON_AUTO,
  UI_THEME_RELOAD_REASON_MANUAL
} UiThemeReloadReason;

static int ui_hover_overlays_allowed(const UiSystem *ui);

static float ui_clampf(float value, float min_value, float max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static void ui_system_emit_action(UiSystem *ui, const EditorAction *action) {
  if (!ui || !action) {
    return;
  }

  editor_action_emit(&ui->action_sink, action);
}

static void ui_system_emit_status(UiSystem *ui, const char *fmt, ...) {
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

static const char *ui_theme_reload_reason_label(UiThemeReloadReason reason) {
  switch (reason) {
  case UI_THEME_RELOAD_REASON_MANUAL:
    return "manually";
  case UI_THEME_RELOAD_REASON_AUTO:
    return "auto";
  case UI_THEME_RELOAD_REASON_STARTUP:
  default:
    return "startup";
  }
}

static void ui_system_sync_menubar_themes(UiSystem *ui) {
  int theme_count = 0;
  int active_theme_index = 0;

  if (!ui || !ui->menu_bar) {
    return;
  }

  theme_count = ui_theme_count();
  if (theme_count < 0) {
    theme_count = 0;
  }
  if (theme_count > UI_THEME_DESCRIPTOR_CACHE_MAX) {
    theme_count = UI_THEME_DESCRIPTOR_CACHE_MAX;
  }

  for (int i = 0; i < theme_count; ++i) {
    const UiThemeDescriptor *descriptor = ui_theme_descriptor_at(i);
    ui->theme_descriptors_cache[i].id = descriptor ? descriptor->id : "";
    ui->theme_descriptors_cache[i].label = descriptor ? descriptor->label : "";
  }

  ui_menubar_set_themes(ui->menu_bar, ui->theme_descriptors_cache, theme_count);

  active_theme_index = ui_theme_index_of_id(ui->active_theme_id);
  if (active_theme_index < 0 || active_theme_index >= theme_count) {
    active_theme_index = ui_theme_index_of_id(ui_theme_default_id());
  }
  if (active_theme_index < 0) {
    active_theme_index = 0;
  }
  ui_menubar_set_active_theme_index(ui->menu_bar, active_theme_index);
}

static int ui_system_set_theme(UiSystem *ui, const char *theme_id,
                               int persist_selection) {
  int theme_index = -1;
  const UiThemeDescriptor *descriptor = NULL;

  if (!ui || !ui->ctx) {
    return 0;
  }

  theme_index = ui_theme_index_of_id(theme_id);
  if (theme_index < 0) {
    theme_index = ui_theme_index_of_id(ui_theme_default_id());
  }
  descriptor = ui_theme_descriptor_at(theme_index);
  if (!descriptor || !descriptor->id) {
    return 0;
  }

  snprintf(ui->active_theme_id, sizeof(ui->active_theme_id), "%s",
           descriptor->id);

  ui->theme = ui_theme_tokens_for_id(ui->active_theme_id);
  ui_theme_apply(ui->ctx, &ui->theme);

  if (ui->menu_bar) {
    ui_menubar_set_height(ui->menu_bar, ui->theme.menu_height);
    ui_system_sync_menubar_themes(ui);
  }

  if (persist_selection) {
    ui_theme_save_selected_id(ui->theme_settings_path, ui->active_theme_id);
  }

  return 1;
}

static void ui_system_reload_themes(UiSystem *ui, int notify_status,
                                    UiThemeReloadReason reason) {
  char previous_theme_id[UI_THEME_ID_CAPACITY];
  int custom_theme_count = 0;
  int fallback_to_default = 0;

  if (!ui) {
    return;
  }

  snprintf(previous_theme_id, sizeof(previous_theme_id), "%s",
           ui->active_theme_id);
  custom_theme_count = ui_theme_reload_external(UI_THEME_DIRECTORY_PATH);
  if (custom_theme_count < 0) {
    ui->theme_directory_signature =
        ui_theme_external_signature(UI_THEME_DIRECTORY_PATH);
    if (notify_status) {
      const char *error_summary = ui_theme_last_reload_error();
      ui_system_emit_status(ui,
                            "Theme reload failed: %s",
                            (error_summary && error_summary[0] != '\0')
                                ? error_summary
                                : "invalid theme file");
    }
    return;
  }

  ui->theme_directory_signature =
      ui_theme_external_signature(UI_THEME_DIRECTORY_PATH);
  ui_system_set_theme(ui, previous_theme_id, 0);
  fallback_to_default = (strcmp(previous_theme_id, ui->active_theme_id) != 0);

  if (notify_status) {
    const char *reason_label = ui_theme_reload_reason_label(reason);
    if (fallback_to_default) {
      ui_system_emit_status(
          ui,
          "Themes %s reloaded (%d custom), active theme missing -> fallback",
          reason_label, custom_theme_count);
    } else {
      ui_system_emit_status(ui, "Themes %s reloaded (%d custom)", reason_label,
                            custom_theme_count);
    }
  }
}

static void ui_system_load_theme_from_settings(UiSystem *ui) {
  char loaded_theme_id[UI_THEME_ID_CAPACITY];

  if (!ui) {
    return;
  }

  if (ui_theme_load_selected_id(ui->theme_settings_path, loaded_theme_id,
                                sizeof(loaded_theme_id))) {
    if (ui_system_set_theme(ui, loaded_theme_id, 0)) {
      return;
    }
  }

  ui_system_set_theme(ui, ui_theme_default_id(), 0);
}

static void ui_system_poll_theme_hot_reload(UiSystem *ui, double now_seconds) {
  unsigned long long signature = 0ull;

  if (!ui) {
    return;
  }

  if ((now_seconds - ui->theme_watch_last_check_seconds) <
      UI_THEME_WATCH_INTERVAL_SECONDS) {
    return;
  }
  ui->theme_watch_last_check_seconds = now_seconds;

  signature = ui_theme_external_signature(UI_THEME_DIRECTORY_PATH);
  if (signature == ui->theme_directory_signature) {
    return;
  }

  ui_system_reload_themes(ui, 1, UI_THEME_RELOAD_REASON_AUTO);
}

static float ui_smoothstep(float t) {
  float clamped = ui_clampf(t, 0.0f, 1.0f);
  return clamped * clamped * (3.0f - 2.0f * clamped);
}

static int ui_rectf_equals(RectF a, RectF b) {
  const float eps = 1e-3f;
  return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps &&
         fabsf(a.w - b.w) <= eps && fabsf(a.h - b.h) <= eps;
}

static RectF ui_clamp_rect_to_window(RectF rect, RectF window_bounds) {
  float max_x = window_bounds.x + window_bounds.w;
  float max_y = window_bounds.y + window_bounds.h;

  if (rect.w < 0.0f) {
    rect.w = 0.0f;
  }
  if (rect.h < 0.0f) {
    rect.h = 0.0f;
  }
  if (rect.x < window_bounds.x) {
    rect.w -= (window_bounds.x - rect.x);
    rect.x = window_bounds.x;
  }
  if (rect.y < window_bounds.y) {
    rect.h -= (window_bounds.y - rect.y);
    rect.y = window_bounds.y;
  }
  if (rect.x > max_x) {
    rect.x = max_x;
  }
  if (rect.y > max_y) {
    rect.y = max_y;
  }
  if (rect.x + rect.w > max_x) {
    rect.w = max_x - rect.x;
  }
  if (rect.y + rect.h > max_y) {
    rect.h = max_y - rect.y;
  }
  if (rect.w < 0.0f) {
    rect.w = 0.0f;
  }
  if (rect.h < 0.0f) {
    rect.h = 0.0f;
  }

  return rect;
}

static void ui_publish_layout(UiSystem *ui, int width, int height) {
  WorkspaceLayout next_layout;
  RectF window_bounds;
  int changed = 0;

  if (!ui) {
    return;
  }

  window_bounds.x = 0.0f;
  window_bounds.y = 0.0f;
  window_bounds.w = (float)width;
  window_bounds.h = (float)height;

  next_layout = ui->layout_snapshot;
  next_layout.window_bounds = window_bounds;
  next_layout.appbar_bounds =
      ui_clamp_rect_to_window(ui->appbar_bounds, window_bounds);
  next_layout.rail_bounds =
      ui_clamp_rect_to_window(ui->rail_bounds, window_bounds);
  next_layout.panel_bounds =
      ui_clamp_rect_to_window(ui->panel_bounds, window_bounds);
  next_layout.status_bounds =
      ui_clamp_rect_to_window(ui->status_bounds, window_bounds);
  next_layout.canvas_content_bounds =
      ui_clamp_rect_to_window(ui->content_bounds, window_bounds);

  changed = !ui_rectf_equals(ui->layout_snapshot.window_bounds,
                             next_layout.window_bounds) ||
            !ui_rectf_equals(ui->layout_snapshot.appbar_bounds,
                             next_layout.appbar_bounds) ||
            !ui_rectf_equals(ui->layout_snapshot.rail_bounds,
                             next_layout.rail_bounds) ||
            !ui_rectf_equals(ui->layout_snapshot.panel_bounds,
                             next_layout.panel_bounds) ||
            !ui_rectf_equals(ui->layout_snapshot.status_bounds,
                             next_layout.status_bounds) ||
            !ui_rectf_equals(ui->layout_snapshot.canvas_content_bounds,
                             next_layout.canvas_content_bounds);

  if (changed) {
    next_layout.layout_revision = ui->layout_snapshot.layout_revision + 1u;
  }
  ui->layout_snapshot = next_layout;
}

static int ui_hover_overlays_allowed(const UiSystem *ui) {
  if (!ui) {
    return 0;
  }

  return !ui->modal_active && !ui_context_menu_is_open(ui) &&
         !ui_menubar_menu_open(ui->menu_bar);
}

static int ui_tool_button(UiSystem *ui, const char *label, int active,
                          const char *tooltip) {
  struct nk_context *ctx = ui->ctx;
  struct nk_rect widget_bounds;
  int pressed = 0;
  int hovered = 0;

  if (!ctx || !label) {
    return 0;
  }

  if (active) {
    struct nk_style_item active_bg = nk_style_item_color(ui->theme.primary);
    nk_style_push_style_item(ctx, &ctx->style.button.normal, active_bg);
    nk_style_push_style_item(ctx, &ctx->style.button.hover, active_bg);
    nk_style_push_style_item(ctx, &ctx->style.button.active, active_bg);
    nk_style_push_color(ctx, &ctx->style.button.text_normal,
                        nk_rgba(255, 255, 255, 255));
    nk_style_push_color(ctx, &ctx->style.button.text_hover,
                        nk_rgba(255, 255, 255, 255));
    nk_style_push_color(ctx, &ctx->style.button.text_active,
                        nk_rgba(255, 255, 255, 255));
  }

  widget_bounds = nk_widget_bounds(ctx);
  pressed = nk_button_label(ctx, label);
  hovered = nk_input_is_mouse_hovering_rect(&ctx->input, widget_bounds);
  if (tooltip && tooltip[0] != '\0' && hovered &&
      ui_hover_overlays_allowed(ui)) {
    nk_tooltip(ctx, tooltip);
  }

  if (active) {
    nk_style_pop_color(ctx);
    nk_style_pop_color(ctx);
    nk_style_pop_color(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
  }

  return pressed;
}

static void ui_tool_rail(UiSystem *ui, const EditorViewModel *view_model,
                         RectF bounds) {
  struct nk_context *ctx = ui->ctx;
  int i = 0;

  ui->rail_bounds = bounds;
  if (!ctx || !view_model || bounds.w <= 0.0f || bounds.h <= 0.0f) {
    return;
  }

  if (nk_begin(ctx, "Tool Rail",
               nk_rect(bounds.x, bounds.y, bounds.w, bounds.h),
               NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
    if (ui->modal_active) {
      nk_widget_disable_begin(ctx);
    }

    nk_layout_row_dynamic(ctx, ui->theme.row_height, 1);
    for (i = 0; i < view_model->tool_count; ++i) {
      const EditorToolView *tool_view = &view_model->tools[i];
      char button_label[96];

      if (!tool_view->id[0]) {
        continue;
      }

      if (tool_view->shortcut[0] != '\0') {
        size_t name_len = strlen(tool_view->name);
        size_t shortcut_len = strlen(tool_view->shortcut);
        if (name_len + shortcut_len + 4u < sizeof(button_label)) {
          memcpy(button_label, tool_view->name, name_len);
          button_label[name_len] = ' ';
          button_label[name_len + 1u] = '(';
          memcpy(button_label + name_len + 2u, tool_view->shortcut, shortcut_len);
          button_label[name_len + shortcut_len + 2u] = ')';
          button_label[name_len + shortcut_len + 3u] = '\0';
        } else {
          snprintf(button_label, sizeof(button_label), "%s", tool_view->name);
        }
      } else {
        snprintf(button_label, sizeof(button_label), "%s", tool_view->name);
      }

      if (!tool_view->available && !ui->modal_active) {
        nk_widget_disable_begin(ctx);
      }
      if (ui_tool_button(ui,
                         button_label,
                         tool_view->active,
                         tool_view->tooltip[0] ? tool_view->tooltip : tool_view->name) &&
          !ui->modal_active && tool_view->available) {
        EditorAction action = editor_action_make_set_tool(tool_view->id);
        ui_system_emit_action(ui, &action);
      }
      if (!tool_view->available && !ui->modal_active) {
        nk_widget_disable_end(ctx);
      }
    }

    nk_layout_row_dynamic(ctx, ui->theme.row_height * 0.9f, 1);
    nk_label(ctx, "", NK_TEXT_LEFT);
    nk_labelf(ctx, NK_TEXT_LEFT, "Active:");
    nk_label(ctx, view_model->summary.active_tool_label, NK_TEXT_LEFT);

    if (ui->modal_active) {
      nk_widget_disable_end(ctx);
    }
  }
  nk_end(ctx);
}

static void ui_inspector_empty_hint(struct nk_context *ctx) {
  nk_label(ctx, "No selection", NK_TEXT_LEFT);
  nk_label(ctx, "Shortcuts: V H L R E", NK_TEXT_LEFT);
  nk_label(ctx, "Document: Ctrl+S save, Ctrl+O load", NK_TEXT_LEFT);
  nk_label(ctx, "Wheel zooms at cursor", NK_TEXT_LEFT);
  nk_label(ctx, "Delete removes selection", NK_TEXT_LEFT);
}

static void ui_inspector_overview(struct nk_context *ctx,
                                  const EditorViewModel *view_model) {
  if (!ctx || !view_model || !view_model->has_selection) {
    return;
  }

  nk_layout_row_dynamic(ctx, 20.0f, 1);
  nk_labelf(ctx, NK_TEXT_LEFT, "Type: %s",
            view_model->summary.selection_type_name);
  nk_labelf(ctx, NK_TEXT_LEFT, "Selected: %d",
            view_model->summary.selection_count);
}

static void ui_property_apply_float(UiSystem *ui, struct nk_context *ctx,
                                    const EditorPropertyView *property_view) {
  float value = 0.0f;
  char value_text[96];

  if (!ui || !ctx || !property_view) {
    return;
  }

  if (!property_view->editable) {
    snprintf(value_text, sizeof(value_text), "%s: %.2f",
             property_view->name, property_view->value);
    nk_label(ctx, value_text, NK_TEXT_LEFT);
    return;
  }

  value = property_view->value;
  nk_property_float(ctx,
                    property_view->name,
                    property_view->min_value,
                    &value,
                    property_view->max_value,
                    property_view->step > 0.0f ? property_view->step : 1.0f,
                    property_view->inc_per_pixel > 0.0f
                        ? property_view->inc_per_pixel
                        : 0.5f);
  if (fabsf(value - property_view->value) > 1e-6f) {
    EditorAction action =
        editor_action_make_modify_property(property_view->object_id,
                                           property_view->name,
                                           value);
    ui_system_emit_action(ui, &action);
  }
}

static void ui_inspector_properties(UiSystem *ui, struct nk_context *ctx,
                                    const EditorViewModel *view_model) {
  int i = 0;

  if (!ctx || !view_model || view_model->property_count <= 0) {
    return;
  }

  nk_layout_row_dynamic(ctx, 20.0f, 1);
  nk_label(ctx, "Properties", NK_TEXT_LEFT);
  nk_layout_row_dynamic(ctx, 24.0f, 1);

  for (i = 0; i < view_model->property_count; ++i) {
    ui_property_apply_float(ui, ctx, &view_model->properties[i]);
  }
}

static const EditorLayerView *ui_find_active_layer_view(const EditorViewModel *view_model) {
  int i = 0;

  if (!view_model) {
    return NULL;
  }

  for (i = 0; i < view_model->layer_count; ++i) {
    if (view_model->layers[i].active) {
      return &view_model->layers[i];
    }
  }

  return NULL;
}

static void ui_sync_layer_name_edit(UiSystem *ui, const EditorLayerView *layer_view) {
  if (!ui || !layer_view) {
    return;
  }

  if (ui->editing_layer_id == layer_view->id) {
    return;
  }

  ui->editing_layer_id = layer_view->id;
  snprintf(ui->layer_name_buffer, sizeof(ui->layer_name_buffer), "%s",
           layer_view->name);
}

static void ui_submit_layer_rename(UiSystem *ui, const EditorLayerView *layer_view) {
  EditorAction action;

  if (!ui || !layer_view || ui->modal_active || ui->layer_name_buffer[0] == '\0' ||
      strcmp(ui->layer_name_buffer, layer_view->name) == 0) {
    return;
  }

  action = editor_action_make_rename_layer(layer_view->id, ui->layer_name_buffer);
  ui_system_emit_action(ui, &action);
}

static void ui_layers_panel(UiSystem *ui, struct nk_context *ctx,
                            const EditorViewModel *view_model) {
  int i = 0;
  const EditorLayerView *active_layer = NULL;

  if (!ui || !ctx || !view_model) {
    return;
  }

  active_layer = ui_find_active_layer_view(view_model);
  if (active_layer) {
    ui_sync_layer_name_edit(ui, active_layer);
  }

  nk_layout_row_dynamic(ctx, 20.0f, 1);
  nk_label(ctx, "Layers", NK_TEXT_LEFT);
  nk_layout_row_begin(ctx, NK_DYNAMIC, 24.0f, 2);
  nk_layout_row_push(ctx, 0.72f);
  nk_label(ctx, "Active layer controls", NK_TEXT_LEFT);
  nk_layout_row_push(ctx, 0.28f);
  if (nk_button_label(ctx, "+ Layer") && !ui->modal_active) {
    EditorAction action = editor_action_make_create_layer("Layer");
    ui_system_emit_action(ui, &action);
  }
  nk_layout_row_end(ctx);

  if (active_layer) {
    nk_layout_row_dynamic(ctx, 20.0f, 1);
    nk_labelf(ctx, NK_TEXT_LEFT, "Active: %s", active_layer->name);

    nk_layout_row_begin(ctx, NK_DYNAMIC, 24.0f, 4);
    nk_layout_row_push(ctx, 0.16f);
    if (nk_button_label(ctx, active_layer->visible ? "Visible" : "Hidden") &&
        !ui->modal_active) {
      EditorAction action =
          editor_action_make_set_layer_visibility(active_layer->id,
                                                  active_layer->visible ? 0 : 1);
      ui_system_emit_action(ui, &action);
    }
    nk_layout_row_push(ctx, 0.16f);
    if (nk_button_label(ctx, active_layer->locked ? "Locked" : "Unlocked") &&
        !ui->modal_active) {
      EditorAction action =
          editor_action_make_set_layer_locked(active_layer->id,
                                              active_layer->locked ? 0 : 1);
      ui_system_emit_action(ui, &action);
    }
    nk_layout_row_push(ctx, 0.48f);
    if (!ui->modal_active) {
      nk_flags edit_result = nk_edit_string_zero_terminated(
          ctx, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_CLIPBOARD,
          ui->layer_name_buffer, (int)sizeof(ui->layer_name_buffer) - 1,
          nk_filter_default);
      if ((edit_result & NK_EDIT_COMMITED) != 0) {
        ui_submit_layer_rename(ui, active_layer);
      }
    } else {
      nk_label(ctx, ui->layer_name_buffer, NK_TEXT_LEFT);
    }
    nk_layout_row_push(ctx, 0.20f);
    if (nk_button_label(ctx, "Apply") && !ui->modal_active) {
      ui_submit_layer_rename(ui, active_layer);
    }
    nk_layout_row_end(ctx);
  }

  for (i = 0; i < view_model->layer_count; ++i) {
    const EditorLayerView* layer_view = &view_model->layers[i];
    char label[96];
    char count_label[24];
    const char* eye = layer_view->visible ? "Hide" : "Show";
    const char* lock = layer_view->locked ? "Unlock" : "Lock";

    snprintf(label, sizeof(label), "%s%s",
             layer_view->active ? "* " : "",
             layer_view->name);
    snprintf(count_label, sizeof(count_label), "%d", layer_view->object_count);

    nk_layout_row_begin(ctx, NK_DYNAMIC, 24.0f, 6);
    nk_layout_row_push(ctx, 0.16f);
    if (nk_button_label(ctx, eye) && !ui->modal_active) {
      EditorAction action =
          editor_action_make_set_layer_visibility(layer_view->id,
                                                  layer_view->visible ? 0 : 1);
      ui_system_emit_action(ui, &action);
    }
    nk_layout_row_push(ctx, 0.16f);
    if (nk_button_label(ctx, lock) && !ui->modal_active) {
      EditorAction action =
          editor_action_make_set_layer_locked(layer_view->id,
                                              layer_view->locked ? 0 : 1);
      ui_system_emit_action(ui, &action);
    }
    nk_layout_row_push(ctx, 0.36f);
    if (nk_button_label(ctx, label) && !ui->modal_active) {
      EditorAction action = editor_action_make_set_active_layer(layer_view->id);
      ui_system_emit_action(ui, &action);
    }
    nk_layout_row_push(ctx, 0.12f);
    nk_label(ctx, count_label, NK_TEXT_RIGHT);
    nk_layout_row_push(ctx, 0.10f);
    if (nk_button_label(ctx, "Up") && !ui->modal_active && i > 0) {
      EditorAction action = editor_action_make_move_layer(layer_view->id, i - 1);
      ui_system_emit_action(ui, &action);
    }
    nk_layout_row_push(ctx, 0.10f);
    if (nk_button_label(ctx, "Dn") && !ui->modal_active &&
        i + 1 < view_model->layer_count) {
      EditorAction action = editor_action_make_move_layer(layer_view->id, i + 1);
      ui_system_emit_action(ui, &action);
    }
    nk_layout_row_end(ctx);
  }
}

static void ui_selection_panel(UiSystem *ui, const EditorViewModel *view_model,
                               RectF bounds) {
  struct nk_context *ctx = ui->ctx;

  ui->panel_bounds = bounds;
  if (!ctx || !view_model || bounds.w <= 0.0f || bounds.h <= 0.0f) {
    return;
  }

  if (nk_begin(
          ctx, "Inspector", nk_rect(bounds.x, bounds.y, bounds.w, bounds.h),
          NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCROLL_AUTO_HIDE)) {
    if (ui->modal_active) {
      nk_widget_disable_begin(ctx);
    }

    nk_layout_row_dynamic(ctx, 20.0f, 1);
    if (!view_model->has_selection) {
      ui_inspector_empty_hint(ctx);
    } else {
      ui_inspector_overview(ctx, view_model);
      ui_inspector_properties(ui, ctx, view_model);
    }
    ui_layers_panel(ui, ctx, view_model);

    if (ui->modal_active) {
      nk_widget_disable_end(ctx);
    }
  }
  nk_end(ctx);
}

static void ui_status_bar(UiSystem *ui, const EditorViewModel *view_model,
                          int window_width, int window_height) {
  struct nk_context *ctx = ui->ctx;
  const float status_h = ui->theme.status_height;
  float status_row_h = ui->theme.row_height * 0.65f;
  char zoom_text[24];

  if (!ctx || !view_model) {
    return;
  }

  ui->status_bounds.x = 0.0f;
  ui->status_bounds.w = (float)window_width;
  ui->status_bounds.h = status_h;
  ui->status_bounds.y = (float)window_height - status_h;

  if (ui->status_bounds.w <= 32.0f || ui->status_bounds.h <= 14.0f) {
    return;
  }

  snprintf(zoom_text, sizeof(zoom_text), "Zoom: %.0f%%",
           view_model->summary.zoom_percent);

  if (nk_begin(ctx, "Status",
               nk_rect(ui->status_bounds.x, ui->status_bounds.y,
                       ui->status_bounds.w, ui->status_bounds.h),
               NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
    nk_layout_row_begin(ctx, NK_DYNAMIC, status_row_h, 5);
    nk_layout_row_push(ctx, 0.12f);
    nk_labelf(ctx, NK_TEXT_LEFT, "Objects: %d", view_model->summary.object_count);
    nk_layout_row_push(ctx, 0.13f);
    nk_label(ctx, zoom_text, NK_TEXT_LEFT);
    nk_layout_row_push(ctx, 0.35f);
    nk_labelf(ctx, NK_TEXT_LEFT, "File: %s%s",
              view_model->summary.current_document_path,
              view_model->summary.document_dirty ? " *" : "");
    nk_layout_row_push(ctx, 0.18f);
    nk_labelf(ctx, NK_TEXT_LEFT, "Undo:%d Redo:%d",
              view_model->summary.undo_count,
              view_model->summary.redo_count);
    nk_layout_row_push(ctx, 0.22f);
    nk_label(ctx, view_model->summary.status_message, NK_TEXT_RIGHT);
    nk_layout_row_end(ctx);
  }
  nk_end(ctx);
}

static void ui_modal_dialogs(UiSystem *ui, const EditorViewModel *view_model,
                             int window_width, int window_height) {
  UiDialogState dialog;
  UiDialogResult result = UI_DIALOG_RESULT_NONE;

  if (!ui || !view_model || view_model->active_dialog.kind == UI_DIALOG_NONE ||
      !ui->ctx) {
    return;
  }

  dialog = view_model->active_dialog;
  if (dialog.kind == UI_DIALOG_SAVE_AS) {
    if (ui->dialog_kind_snapshot != dialog.kind) {
      snprintf(ui->dialog_input_buffer, sizeof(ui->dialog_input_buffer), "%s",
               dialog.payload.text);
      ui->dialog_kind_snapshot = dialog.kind;
    }
    snprintf(dialog.payload.text, sizeof(dialog.payload.text), "%s",
             ui->dialog_input_buffer);
  } else {
    ui->dialog_kind_snapshot = dialog.kind;
  }

  result = ui_dialog_show(ui->ctx, &dialog, window_width, window_height);
  if (dialog.kind == UI_DIALOG_SAVE_AS) {
    snprintf(ui->dialog_input_buffer, sizeof(ui->dialog_input_buffer), "%s",
             dialog.payload.text);
  }
  if (result != UI_DIALOG_RESULT_NONE) {
    EditorAction action =
        editor_action_make_resolve_dialog(result, dialog.payload.text);
    ui_system_emit_action(ui, &action);
  }
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
