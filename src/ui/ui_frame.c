/**
 * @file ui_frame.c
 * @brief Per-frame UI composition and layout publishing.
 */
#include <ui/ui_system.h>

#include "ui/ui_context_menu.h"
#include "ui/ui_system_internal.h"

#define UI_MIN_CANVAS_WIDTH 320.0f

static void ui_system_update_modal_state(UiSystem *ui,
                                         const EditorViewModel *view_model) {
  ui->last_selection_count = view_model->summary.selection_count;
  ui->modal_active = view_model->active_dialog.kind != UI_DIALOG_NONE &&
                     view_model->active_dialog.modal;
  if (ui->modal_active) {
    ui_context_menu_reset(ui);
  } else {
    ui->dialog_kind_snapshot = UI_DIALOG_NONE;
  }
}

static float ui_system_begin_build_timing(UiSystem *ui) {
  double now_seconds = platform_time_seconds();
  float dt_seconds = (float)(now_seconds - ui->last_frame_seconds);

  ui->last_frame_seconds = now_seconds;
  dt_seconds = ui_clampf(dt_seconds, 0.0f, 0.10f);
  ui_system_poll_theme_hot_reload(ui, now_seconds);
  return dt_seconds;
}

static void ui_system_build_menu_region(UiSystem *ui,
                                        const EditorViewModel *view_model,
                                        int width) {
  int requested_theme_index = -1;
  int requested_theme_reload = 0;
  const UiThemeDescriptor *requested_theme = NULL;

  if (!ui->menu_bar || ui->modal_active) {
    return;
  }

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

static void ui_system_update_inspector_animation(UiSystem *ui,
                                                int inspector_target_visible,
                                                float dt_seconds) {
  float anim_step = 0.0f;

  ui->inspector_target_visible = inspector_target_visible;
  if (!ui->inspector_anim_initialized) {
    ui->inspector_anim_t = inspector_target_visible ? 1.0f : 0.0f;
    ui->inspector_anim_initialized = 1;
    return;
  }

  if (!ui->theme.enable_transitions ||
      ui->theme.transition_duration <= 1e-4f) {
    ui->inspector_anim_t = inspector_target_visible ? 1.0f : 0.0f;
    return;
  }

  anim_step = dt_seconds / ui->theme.transition_duration;
  if (ui->inspector_target_visible) {
    ui->inspector_anim_t =
        ui_clampf(ui->inspector_anim_t + anim_step, 0.0f, 1.0f);
  } else {
    ui->inspector_anim_t =
        ui_clampf(ui->inspector_anim_t - anim_step, 0.0f, 1.0f);
  }
}

static void ui_system_build_inspector_region(UiSystem *ui,
                                             const EditorViewModel *view_model,
                                             int width,
                                             float content_top,
                                             float content_height,
                                             float dt_seconds) {
  int inspector_requested = ui_menubar_inspector_visible(ui->menu_bar);
  float needed_width = ui->theme.tool_rail_width + UI_MIN_CANVAS_WIDTH;
  int inspector_target_visible = 0;
  int inspector_render_visible = 0;
  RectF inspector_bounds = {0};

  if (inspector_requested) {
    needed_width += ui->theme.panel_width;
  }
  inspector_target_visible =
      inspector_requested && ((float)width >= needed_width);
  ui_system_update_inspector_animation(ui, inspector_target_visible, dt_seconds);

  inspector_render_visible = (ui->inspector_anim_t > 1e-3f);
  if (inspector_render_visible) {
    float inspector_eased_t = ui_smoothstep(ui->inspector_anim_t);
    float inspector_hidden_x = (float)width;
    float inspector_shown_x = 0.0f;

    inspector_bounds.w = ui->theme.panel_width;
    inspector_bounds.h = content_height;
    inspector_shown_x = (float)width - inspector_bounds.w;
    inspector_bounds.x =
        inspector_hidden_x +
        (inspector_shown_x - inspector_hidden_x) * inspector_eased_t;
    inspector_bounds.y = content_top;
    ui->panel_bounds = inspector_bounds;
    ui_selection_panel(ui, view_model, inspector_bounds);
    return;
  }

  ui->panel_bounds.x = 0.0f;
  ui->panel_bounds.y = 0.0f;
  ui->panel_bounds.w = 0.0f;
  ui->panel_bounds.h = 0.0f;
}

static void ui_system_update_canvas_content_bounds(UiSystem *ui, int width) {
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
}

void ui_system_build(UiSystem *ui, const EditorViewModel *view_model) {
  int width = 0;
  int height = 0;
  float content_top = 0.0f;
  float content_bottom = 0.0f;
  float content_height = 0.0f;
  RectF rail_bounds = {0};
  float dt_seconds;

  if (!ui || !view_model) {
    return;
  }

  ui_system_update_modal_state(ui, view_model);

  if (ui->window) {
    platform_window_get_size(ui->window, &width, &height);
  }

  if (width <= 0 || height <= 0) {
    ui_publish_layout(ui, 1, 1);
    return;
  }

  dt_seconds = ui_system_begin_build_timing(ui);
  ui_system_build_menu_region(ui, view_model, width);

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

  ui_system_build_inspector_region(ui,
                                   view_model,
                                   width,
                                   content_top,
                                   content_height,
                                   dt_seconds);

  ui_status_bar(ui, view_model, width, height);
  ui_modal_dialogs(ui, view_model, width, height);

  ui_system_update_canvas_content_bounds(ui, width);

  if (!ui->modal_active) {
    ui_context_menu_build(ui, view_model, &ui->action_sink);
  }

  ui_publish_layout(ui, width, height);
}
