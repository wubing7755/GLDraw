/**
 * @file ui_system.c
 * @brief Nuklear UI system implementation (menu, tool rail, inspector, status).
 *
 * Role in project:
 * - Builds all editor UI panels and theme behavior each frame.
 * - Publishes layout bounds that drive canvas viewport and pointer blocking.
 *
 * Module relationships:
 * - Uses workspace/document/tools for editable UI actions.
 * - Cooperates with application input loop and theme subsystem.
 */
#include <ui/ui_system.h>
#include <ui/ui_menubar.h>
#include <ui/ui_dialog.h>

#include <app/workspace.h>
#include <app/workspace_actions.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <glad/glad.h>
#include <tools/tool_controller.h>

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
#include <ui/ui_theme.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UI_MAX_VERTEX_BUFFER (512 * 1024)
#define UI_MAX_ELEMENT_BUFFER (128 * 1024)
#define UI_MIN_CANVAS_WIDTH 320.0f
#define UI_THEME_ID_CAPACITY 64
#define UI_THEME_SETTINGS_PATH "gldraw.settings.json"
#define UI_THEME_DIRECTORY_PATH "themes"
#define UI_THEME_DESCRIPTOR_CACHE_MAX 64
#define UI_THEME_WATCH_INTERVAL_SECONDS 0.35

struct UiSystem {
  struct nk_glfw glfw;
  struct nk_context *ctx;
  GLFWwindow *window_handle;
  UiMenuBar *menu_bar;
  UiThemeTokens theme;
  char active_theme_id[UI_THEME_ID_CAPACITY];
  char theme_settings_path[260];
  UiThemeDescriptor theme_descriptors_cache[UI_THEME_DESCRIPTOR_CACHE_MAX];
  unsigned long long theme_directory_signature;
  double theme_watch_last_check_seconds;
  RectF appbar_bounds;
  RectF rail_bounds;
  RectF panel_bounds;
  RectF status_bounds;
  RectF content_bounds;
  WorkspaceLayout layout_snapshot;
  float inspector_anim_t;
  int inspector_target_visible;
  int inspector_anim_initialized;
  int inspector_edit_active;
  DocumentSnapshot inspector_edit_before_snapshot;
  int modal_active;
  double last_frame_seconds;
};

typedef enum UiThemeReloadReason {
  UI_THEME_RELOAD_REASON_STARTUP = 0,
  UI_THEME_RELOAD_REASON_AUTO,
  UI_THEME_RELOAD_REASON_MANUAL
} UiThemeReloadReason;

/**
 * @brief Clamps a float value.
 * @param value Value to clamp.
 * @param min_value Minimum value.
 * @param max_value Maximum value.
 * @return Clamped value.
 */
static float ui_clampf(float value, float min_value, float max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

/**
 * @brief Gets label for theme reload reason.
 * @param reason Reload reason enum.
 * @return Reason label string.
 */
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

/**
 * @brief Syncs theme registry with menu bar.
 * @param ui UI system instance.
 * @return None.
 */
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

/**
 * @brief Sets the active theme.
 * @param ui UI system instance.
 * @param theme_id Theme ID to activate.
 * @param persist_selection Whether to persist selection.
 * @return 1 on success, 0 on failure.
 */
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

/**
 * @brief Reloads external themes.
 * @param ui UI system instance.
 * @param workspace Workspace instance.
 * @param notify_status Whether to notify status bar.
 * @param reason Reload reason.
 * @return None.
 */
static void ui_system_reload_themes(UiSystem *ui, Workspace *workspace,
                                    int notify_status,
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
    if (notify_status && workspace) {
      const char *error_summary = ui_theme_last_reload_error();
      snprintf(workspace->status_message, sizeof(workspace->status_message),
               "Theme reload failed: %s",
               (error_summary && error_summary[0] != '\0')
                   ? error_summary
                   : "invalid theme file");
    }
    return;
  }

  ui->theme_directory_signature =
      ui_theme_external_signature(UI_THEME_DIRECTORY_PATH);

  /* Keep current theme id if still present; otherwise fall back through
   * ui_system_set_theme. */
  ui_system_set_theme(ui, previous_theme_id, 0);
  fallback_to_default = (strcmp(previous_theme_id, ui->active_theme_id) != 0);

  if (notify_status && workspace) {
    const char *reason_label = ui_theme_reload_reason_label(reason);
    if (fallback_to_default) {
      snprintf(
          workspace->status_message, sizeof(workspace->status_message),
          "Themes %s reloaded (%d custom), active theme missing -> fallback",
          reason_label, custom_theme_count);
    } else {
      snprintf(workspace->status_message, sizeof(workspace->status_message),
               "Themes %s reloaded (%d custom)", reason_label,
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

static void ui_system_poll_theme_hot_reload(UiSystem *ui, Workspace *workspace,
                                            double now_seconds) {
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

  ui_system_reload_themes(ui, workspace, 1, UI_THEME_RELOAD_REASON_AUTO);
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

static void ui_publish_layout(UiSystem *ui, Workspace *workspace, int width,
                              int height) {
  WorkspaceLayout next_layout;
  RectF window_bounds;
  int changed = 0;

  if (!ui || !workspace) {
    return;
  }

  window_bounds.x = 0.0f;
  window_bounds.y = 0.0f;
  window_bounds.w = (float)width;
  window_bounds.h = (float)height;

  next_layout = workspace->layout;
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

  changed = !ui_rectf_equals(workspace->layout.window_bounds,
                             next_layout.window_bounds) ||
            !ui_rectf_equals(workspace->layout.appbar_bounds,
                             next_layout.appbar_bounds) ||
            !ui_rectf_equals(workspace->layout.rail_bounds,
                             next_layout.rail_bounds) ||
            !ui_rectf_equals(workspace->layout.panel_bounds,
                             next_layout.panel_bounds) ||
            !ui_rectf_equals(workspace->layout.status_bounds,
                             next_layout.status_bounds) ||
            !ui_rectf_equals(workspace->layout.canvas_content_bounds,
                             next_layout.canvas_content_bounds);

  if (changed) {
    next_layout.layout_revision = workspace->layout.layout_revision + 1u;
    workspace->layout = next_layout;
  }

  ui->layout_snapshot = workspace->layout;
}

static ToolContext ui_tool_context(Workspace *workspace) {
  ToolContext context;
  context.workspace = workspace;
  context.document = &workspace->document;
  context.history = &workspace->history;
  context.canvas = &workspace->canvas;
  return context;
}

static void ui_reset_inspector_edit_session(UiSystem *ui) {
  if (!ui) {
    return;
  }

  document_snapshot_free(&ui->inspector_edit_before_snapshot);
  document_snapshot_init(&ui->inspector_edit_before_snapshot);
  ui->inspector_edit_active = 0;
}

static int ui_begin_inspector_edit_session(UiSystem *ui, Workspace *workspace) {
  if (!ui || !workspace) {
    return 0;
  }

  if (ui->inspector_edit_active) {
    return 1;
  }

  document_snapshot_init(&ui->inspector_edit_before_snapshot);
  if (!document_snapshot_capture(&ui->inspector_edit_before_snapshot,
                                 &workspace->document)) {
    ui_reset_inspector_edit_session(ui);
    return 0;
  }

  ui->inspector_edit_active = 1;
  return 1;
}

static void ui_finalize_inspector_edit_session(UiSystem *ui,
                                               Workspace *workspace) {
  if (!ui || !workspace || !ui->inspector_edit_active) {
    return;
  }

  if (workspace->document.revision !=
      ui->inspector_edit_before_snapshot.revision) {
    if (!document_history_push(&workspace->history,
                               &ui->inspector_edit_before_snapshot,
                               &workspace->document)) {
      ui_reset_inspector_edit_session(ui);
      workspace_sync_document_dirty(workspace);
      return;
    }
  } else {
    document_snapshot_free(&ui->inspector_edit_before_snapshot);
    document_snapshot_init(&ui->inspector_edit_before_snapshot);
  }

  ui->inspector_edit_active = 0;
  workspace_sync_document_dirty(workspace);
}

static void ui_apply_scalar_edit(UiSystem *ui, Workspace *workspace,
                                 GraphicObject *object, const char *key,
                                 float before_value, float after_value) {

  if (!ui || !workspace || !object || !key || key[0] == '\0') {
    return;
  }
  if (fabsf(after_value - before_value) <= 1e-6f) {
    return;
  }

  if (!ui_begin_inspector_edit_session(ui, workspace)) {
    return;
  }

  if (!object_set_scalar(object, key, after_value)) {
    return;
  }

  workspace_sync_document_dirty(workspace);
}

static void ui_apply_stroke_color(UiSystem *ui, Workspace *workspace,
                                  GraphicObject *object,
                                  Color color) {
  Color before;

  if (!ui || !workspace || !object) {
    return;
  }

  before = object->style.stroke_color;
  if (fabsf(color.r - before.r) > 1e-6f) {
    ui_apply_scalar_edit(ui, workspace, object, "stroke_r", before.r, color.r);
  }
  if (fabsf(color.g - before.g) > 1e-6f) {
    ui_apply_scalar_edit(ui, workspace, object, "stroke_g", before.g, color.g);
  }
  if (fabsf(color.b - before.b) > 1e-6f) {
    ui_apply_scalar_edit(ui, workspace, object, "stroke_b", before.b, color.b);
  }
  if (fabsf(color.a - before.a) > 1e-6f) {
    ui_apply_scalar_edit(ui, workspace, object, "stroke_a", before.a, color.a);
  }
}

static void ui_apply_stroke_width(UiSystem *ui, Workspace *workspace,
                                  GraphicObject *object, float stroke_width) {
  float before = 0.0f;

  if (!ui || !workspace || !object) {
    return;
  }

  before = object->style.stroke_width;
  ui_apply_scalar_edit(ui, workspace, object, "stroke_width", before,
                       stroke_width);
}

static void ui_property_apply_float(UiSystem *ui, struct nk_context *ctx,
                                    Workspace *workspace, GraphicObject *object,
                                    const char *label, const char *key,
                                    float min_value, float *value,
                                    float max_value, float step,
                                    float inc_per_pixel) {
  float before = *value;

  nk_property_float(ctx, label, min_value, value, max_value, step,
                    inc_per_pixel);
  if (fabsf(*value - before) > 1e-6f) {
    ui_apply_scalar_edit(ui, workspace, object, key, before, *value);
  }
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
  if (tooltip && tooltip[0] != '\0' && hovered) {
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

static void ui_tool_rail(UiSystem *ui, Workspace *workspace, RectF bounds) {
  struct nk_context *ctx = ui->ctx;
  ToolContext context = ui_tool_context(workspace);
  ToolKind active = workspace->tools.active_kind;

  ui->rail_bounds = bounds;
  if (bounds.w <= 0.0f || bounds.h <= 0.0f) {
    return;
  }

  if (nk_begin(ctx, "Tool Rail",
               nk_rect(bounds.x, bounds.y, bounds.w, bounds.h),
               NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
    nk_layout_row_dynamic(ctx, ui->theme.row_height, 1);

    if (ui_tool_button(ui, "Select (V)", active == TOOL_KIND_SELECT,
                       "Select and edit objects")) {
      tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_SELECT);
    }
    if (ui_tool_button(ui, "Hand (H)", active == TOOL_KIND_PAN,
                       "Pan canvas view")) {
      tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_PAN);
    }
    if (ui_tool_button(ui, "Line (L)", active == TOOL_KIND_LINE, "Draw line")) {
      tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_LINE);
    }
    if (ui_tool_button(ui, "Rect (R)", active == TOOL_KIND_RECT,
                       "Draw rectangle")) {
      tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_RECT);
    }
    if (ui_tool_button(ui, "Ellipse (E)", active == TOOL_KIND_ELLIPSE,
                       "Draw ellipse")) {
      tool_controller_set_active(&workspace->tools, &context,
                                 TOOL_KIND_ELLIPSE);
    }

    nk_layout_row_dynamic(ctx, ui->theme.row_height * 0.9f, 1);
    nk_label(ctx, "", NK_TEXT_LEFT);
    nk_labelf(ctx, NK_TEXT_LEFT, "Active:");
    nk_label(ctx, tool_controller_active_label(&workspace->tools),
             NK_TEXT_LEFT);
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
                                  const Document *document,
                                  const GraphicObject *object) {
  if (!ctx || !document || !object) {
    return;
  }

  nk_layout_row_dynamic(ctx, 20.0f, 1);
  nk_labelf(ctx, NK_TEXT_LEFT, "Type: %s", object_type_name(object->type));
  nk_labelf(ctx, NK_TEXT_LEFT, "Selected: %d", document->selection.count);
}

static void ui_inspector_style(UiSystem *ui, struct nk_context *ctx,
                               Workspace *workspace,
                               GraphicObject *object) {
  Color stroke;
  float stroke_width;

  if (!ctx || !workspace || !object) {
    return;
  }

  stroke = object->style.stroke_color;
  stroke_width = object->style.stroke_width;

  nk_layout_row_dynamic(ctx, 20.0f, 1);
  nk_label(ctx, "Style", NK_TEXT_LEFT);
  nk_layout_row_dynamic(ctx, 22.0f, 2);
  nk_label(ctx, "Stroke R", NK_TEXT_LEFT);
  if (nk_slider_float(ctx, 0.0f, &stroke.r, 1.0f, 0.01f))
    ui_apply_stroke_color(ui, workspace, object, stroke);
  nk_label(ctx, "Stroke G", NK_TEXT_LEFT);
  if (nk_slider_float(ctx, 0.0f, &stroke.g, 1.0f, 0.01f))
    ui_apply_stroke_color(ui, workspace, object, stroke);
  nk_label(ctx, "Stroke B", NK_TEXT_LEFT);
  if (nk_slider_float(ctx, 0.0f, &stroke.b, 1.0f, 0.01f))
    ui_apply_stroke_color(ui, workspace, object, stroke);
  nk_label(ctx, "Stroke A", NK_TEXT_LEFT);
  if (nk_slider_float(ctx, 0.1f, &stroke.a, 1.0f, 0.01f))
    ui_apply_stroke_color(ui, workspace, object, stroke);
  nk_label(ctx, "Line Width", NK_TEXT_LEFT);
  if (nk_slider_float(ctx, 1.0f, &stroke_width, 12.0f, 0.1f))
    ui_apply_stroke_width(ui, workspace, object, stroke_width);
}

static void ui_inspector_geometry(UiSystem *ui, struct nk_context *ctx,
                                  Workspace *workspace,
                                  GraphicObject *object) {
  if (!ctx || !workspace || !object) {
    return;
  }

  if (object->type == GRAPHIC_OBJECT_LINE) {
    float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
    object_get_scalar(object, "x1", &x1);
    object_get_scalar(object, "y1", &y1);
    object_get_scalar(object, "x2", &x2);
    object_get_scalar(object, "y2", &y2);
    nk_layout_row_dynamic(ctx, 20.0f, 1);
    nk_label(ctx, "Geometry", NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 24.0f, 1);
    ui_property_apply_float(ui, ctx, workspace, object, "#X1", "x1", -5000.0f, &x1,
                            5000.0f, 1.0f, 0.5f);
    ui_property_apply_float(ui, ctx, workspace, object, "#Y1", "y1", -5000.0f, &y1,
                            5000.0f, 1.0f, 0.5f);
    ui_property_apply_float(ui, ctx, workspace, object, "#X2", "x2", -5000.0f, &x2,
                            5000.0f, 1.0f, 0.5f);
    ui_property_apply_float(ui, ctx, workspace, object, "#Y2", "y2", -5000.0f, &y2,
                            5000.0f, 1.0f, 0.5f);
  } else {
    float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
    object_get_scalar(object, "x", &x);
    object_get_scalar(object, "y", &y);
    object_get_scalar(object, "width", &width);
    object_get_scalar(object, "height", &height);
    nk_layout_row_dynamic(ctx, 20.0f, 1);
    nk_label(ctx, "Bounds", NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 24.0f, 1);
    ui_property_apply_float(ui, ctx, workspace, object, "#X", "x", -5000.0f, &x,
                            5000.0f, 1.0f, 0.5f);
    ui_property_apply_float(ui, ctx, workspace, object, "#Y", "y", -5000.0f, &y,
                            5000.0f, 1.0f, 0.5f);
    ui_property_apply_float(ui, ctx, workspace, object, "#W", "width", 1.0f, &width,
                            5000.0f, 1.0f, 0.5f);
    ui_property_apply_float(ui, ctx, workspace, object, "#H", "height", 1.0f,
                            &height, 5000.0f, 1.0f, 0.5f);
  }
}

static void ui_selection_panel(UiSystem *ui, Workspace *workspace,
                               RectF bounds) {
  struct nk_context *ctx = ui->ctx;
  Document *document = &workspace->document;
  GraphicObject *object = document_primary_selection(document);

  ui->panel_bounds = bounds;
  if (bounds.w <= 0.0f || bounds.h <= 0.0f) {
    return;
  }

  if (nk_begin(
          ctx, "Inspector", nk_rect(bounds.x, bounds.y, bounds.w, bounds.h),
          NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCROLL_AUTO_HIDE)) {
    nk_layout_row_dynamic(ctx, 20.0f, 1);
    if (!object) {
      ui_inspector_empty_hint(ctx);
    } else {
      ui_inspector_overview(ctx, document, object);
      ui_inspector_style(ui, ctx, workspace, object);
      ui_inspector_geometry(ui, ctx, workspace, object);
    }
  }
  nk_end(ctx);
}

static void ui_status_bar(UiSystem *ui, Workspace *workspace, int window_width,
                          int window_height) {
  struct nk_context *ctx = ui->ctx;
  const float status_h = ui->theme.status_height;
  const char *status_text =
      workspace->status_message[0] ? workspace->status_message : "Ready";
  float status_row_h = ui->theme.row_height * 0.65f;
  char zoom_text[24];

  ui->status_bounds.x = 0.0f;
  ui->status_bounds.w = (float)window_width;
  ui->status_bounds.h = status_h;
  ui->status_bounds.y = (float)window_height - status_h;

  if (ui->status_bounds.w <= 32.0f || ui->status_bounds.h <= 14.0f) {
    return;
  }

  snprintf(zoom_text, sizeof(zoom_text), "Zoom: %.0f%%",
           canvas_view_zoom(&workspace->canvas) * 100.0f);

  if (nk_begin(ctx, "Status",
               nk_rect(ui->status_bounds.x, ui->status_bounds.y,
                       ui->status_bounds.w, ui->status_bounds.h),
               NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
    nk_layout_row_begin(ctx, NK_DYNAMIC, status_row_h, 5);
    nk_layout_row_push(ctx, 0.12f);
    nk_labelf(ctx, NK_TEXT_LEFT, "Objects: %d", workspace->document.count);
    nk_layout_row_push(ctx, 0.13f);
    nk_label(ctx, zoom_text, NK_TEXT_LEFT);
    nk_layout_row_push(ctx, 0.35f);
    nk_labelf(ctx, NK_TEXT_LEFT, "File: %s%s",
              workspace->current_document_path[0]
                  ? workspace->current_document_path
                  : "(default)",
              workspace->document_dirty ? " *" : "");
    nk_layout_row_push(ctx, 0.18f);
    nk_labelf(ctx, NK_TEXT_LEFT, "Undo:%d Redo:%d",
              workspace->history.undo_count, workspace->history.redo_count);
    nk_layout_row_push(ctx, 0.22f);
    nk_label(ctx, status_text, NK_TEXT_RIGHT);
    nk_layout_row_end(ctx);
  }
  nk_end(ctx);
}

/**
 * @brief Render active reusable workspace dialogs and route the result back into workspace state.
 * @param ui UI system instance.
 * @param workspace Workspace instance.
 * @param window_width Window width in pixels.
 * @param window_height Window height in pixels.
 * @return None.
 */
static void ui_modal_dialogs(UiSystem *ui, Workspace *workspace,
                             int window_width, int window_height) {
  UiDialogResult result = UI_DIALOG_RESULT_NONE;

  if (!ui || !workspace || !workspace_modal_is_active(workspace) || !ui->ctx) {
    return;
  }

  result =
      ui_dialog_show(ui->ctx, &workspace->active_dialog, window_width, window_height);
  if (result != UI_DIALOG_RESULT_NONE) {
    workspace_resolve_active_dialog(workspace, result);
  }
}

UiSystem *ui_system_create(PlatformWindow *window) {
  UiSystem *ui = (UiSystem *)calloc(1, sizeof(*ui));
  struct nk_font_atlas *atlas = NULL;

  if (!ui) {
    return NULL;
  }

  ui->ctx = nk_glfw3_init(&ui->glfw, window->handle, 0);
  if (!ui->ctx) {
    free(ui);
    return NULL;
  }

  nk_glfw3_font_stash_begin(&ui->glfw, &atlas);
  nk_glfw3_font_stash_end(&ui->glfw);

  snprintf(ui->theme_settings_path, sizeof(ui->theme_settings_path), "%s",
           UI_THEME_SETTINGS_PATH);

  ui_system_reload_themes(ui, NULL, 0, UI_THEME_RELOAD_REASON_STARTUP);
  ui->theme_watch_last_check_seconds = glfwGetTime();

  ui->menu_bar = ui_menubar_create(ui->ctx);
  if (ui->menu_bar) {
    ui_system_sync_menubar_themes(ui);
  }
  ui_system_load_theme_from_settings(ui);

  ui->inspector_anim_t = 1.0f;
  ui->inspector_target_visible = 1;
  ui->inspector_anim_initialized = 0;
  document_snapshot_init(&ui->inspector_edit_before_snapshot);
  ui->last_frame_seconds = glfwGetTime();
  ui->window_handle = window->handle;

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
  document_snapshot_free(&ui->inspector_edit_before_snapshot);
  nk_glfw3_shutdown(&ui->glfw);
  free(ui);
}

void ui_system_begin_frame(UiSystem *ui) {
  if (ui) {
    nk_glfw3_new_frame(&ui->glfw);
  }
}

void ui_system_build(UiSystem *ui, Workspace *workspace) {
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

  if (!ui || !workspace) {
    return;
  }

  ui->modal_active = workspace_modal_is_active(workspace);

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
    if (workspace) {
      ui_publish_layout(ui, workspace, 1, 1);
    }
    return;
  }
  now_seconds = glfwGetTime();
  dt_seconds = (float)(now_seconds - ui->last_frame_seconds);
  ui->last_frame_seconds = now_seconds;
  dt_seconds = ui_clampf(dt_seconds, 0.0f, 0.10f);
  ui_system_poll_theme_hot_reload(ui, workspace, now_seconds);

  if (ui->menu_bar && !ui->modal_active) {
    int requested_theme_index = -1;
    int requested_theme_reload = 0;
    const UiThemeDescriptor *requested_theme = NULL;

    ui_menubar_set_height(ui->menu_bar, ui->theme.menu_height);
    ui_menubar_build(ui->menu_bar, workspace, width);

    requested_theme_reload = ui_menubar_take_theme_reload_request(ui->menu_bar);
    if (requested_theme_reload) {
      ui_system_reload_themes(ui, workspace, 1, UI_THEME_RELOAD_REASON_MANUAL);
    }

    requested_theme_index = ui_menubar_take_theme_request(ui->menu_bar);
    if (requested_theme_index >= 0) {
      requested_theme = ui_theme_descriptor_at(requested_theme_index);
      if (requested_theme && ui_system_set_theme(ui, requested_theme->id, 1)) {
        snprintf(workspace->status_message, sizeof(workspace->status_message),
                 "Theme: %s",
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
  if (!ui->modal_active) {
    ui_tool_rail(ui, workspace, rail_bounds);
  }

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
    if (!ui->modal_active) {
      ui_selection_panel(ui, workspace, inspector_bounds);
    }
  } else {
    ui->panel_bounds.x = 0.0f;
    ui->panel_bounds.y = 0.0f;
    ui->panel_bounds.w = 0.0f;
    ui->panel_bounds.h = 0.0f;
  }

  ui_status_bar(ui, workspace, width, height);
  ui_modal_dialogs(ui, workspace, width, height);

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

  ui_publish_layout(ui, workspace, width, height);

  if (ui->inspector_edit_active && !ui_system_has_active_interaction(ui)) {
    ui_finalize_inspector_edit_session(ui, workspace);
  }
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

  return nk_item_is_any_active(ui->ctx);
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
