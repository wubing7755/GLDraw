/**
 * @file ui_chrome.c
 * @brief UiSystem tool rail and status bar rendering.
 */
#include "ui/ui_context_menu.h"
#include "ui/ui_system_internal.h"

#include <ui/ui_menubar.h>

#include <stdio.h>
#include <string.h>

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

void ui_tool_rail(UiSystem *ui, const EditorViewModel *view_model,
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
          memcpy(button_label + name_len + 2u, tool_view->shortcut,
                 shortcut_len);
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
      if (ui_tool_button(ui, button_label, tool_view->active,
                         tool_view->tooltip[0] ? tool_view->tooltip
                                               : tool_view->name) &&
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

void ui_status_bar(UiSystem *ui, const EditorViewModel *view_model,
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
    nk_labelf(ctx, NK_TEXT_LEFT, "Objects: %d",
              view_model->summary.object_count);
    nk_layout_row_push(ctx, 0.13f);
    nk_label(ctx, zoom_text, NK_TEXT_LEFT);
    nk_layout_row_push(ctx, 0.35f);
    nk_labelf(ctx, NK_TEXT_LEFT, "File: %s%s",
              view_model->summary.current_document_path,
              view_model->summary.document_dirty ? " *" : "");
    nk_layout_row_push(ctx, 0.18f);
    nk_labelf(ctx, NK_TEXT_LEFT, "Undo:%d Redo:%d",
              view_model->summary.undo_count, view_model->summary.redo_count);
    nk_layout_row_push(ctx, 0.22f);
    nk_label(ctx, view_model->summary.status_message, NK_TEXT_RIGHT);
    nk_layout_row_end(ctx);
  }
  nk_end(ctx);
}
