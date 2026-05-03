/**
 * @file ui_context_menu.c
 * @brief Canvas context menu implementation.
 */
#include "ui/ui_context_menu.h"
#include "ui/ui_system_internal.h"

#include <app/command_registry.h>
#include <base/math2d.h>

#include <stdio.h>
#include <string.h>

#define UI_CONTEXT_MENU_WIDTH 220.0f
#define UI_CONTEXT_MENU_HEIGHT 240.0f
#define UI_CONTEXT_MENU_ANCHOR_SIZE 1.0f

static float ui_context_menu_clampf(float value, float min_value, float max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static RectF ui_context_menu_popup_bounds(const UiContextMenuState *menu) {
  RectF bounds = {0.0f, 0.0f, UI_CONTEXT_MENU_WIDTH, UI_CONTEXT_MENU_HEIGHT};

  if (!menu) {
    return bounds;
  }

  bounds.x = ui_context_menu_clampf(
      menu->anchor_screen.x, menu->canvas_bounds.x,
      menu->canvas_bounds.x + menu->canvas_bounds.w - UI_CONTEXT_MENU_WIDTH);
  bounds.y = ui_context_menu_clampf(
      menu->anchor_screen.y, menu->canvas_bounds.y,
      menu->canvas_bounds.y + menu->canvas_bounds.h - UI_CONTEXT_MENU_HEIGHT);
  return bounds;
}

static void ui_context_menu_open(UiSystem *ui, Vec2 screen_pos) {
  RectF canvas_bounds;

  if (!ui) {
    return;
  }

  canvas_bounds = ui->layout_snapshot.canvas_content_bounds;
  if (canvas_bounds.w <= 1.0f || canvas_bounds.h <= 1.0f) {
    canvas_bounds = ui->content_bounds;
  }

  if (canvas_bounds.w <= 0.0f || canvas_bounds.h <= 0.0f ||
      !rectf_contains_point(&canvas_bounds, screen_pos)) {
    return;
  }

  ui->context_menu.open = 1;
  ui->context_menu.close_requested = 0;
  ui->context_menu.mode = ui->last_selection_count > 0
                              ? UI_CONTEXT_MENU_MODE_SELECTION
                              : UI_CONTEXT_MENU_MODE_TOOLS;
  ui->context_menu.anchor_screen = screen_pos;
  ui->context_menu.canvas_bounds = canvas_bounds;
  ui->context_menu.popup_bounds = ui_context_menu_popup_bounds(&ui->context_menu);
}

static void ui_context_menu_format_command_label(const EditorViewModel *view_model,
                                                 const char *label,
                                                 EditorCommand command,
                                                 char *buffer,
                                                 size_t buffer_size) {
  const char *shortcut = "";

  if (!buffer || buffer_size == 0u) {
    return;
  }

  buffer[0] = '\0';
  if (!label) {
    return;
  }

  if (view_model && command > EDITOR_COMMAND_NONE) {
    shortcut = editor_viewmodel_command_shortcut(view_model, command);
  }

  if (shortcut && shortcut[0] != '\0') {
    snprintf(buffer, buffer_size, "%-18s %s", label, shortcut);
  } else {
    snprintf(buffer, buffer_size, "%s", label);
  }
}

static int ui_context_menu_item(UiSystem *ui, const char *label,
                                EditorCommand command,
                                const EditorViewModel *view_model,
                                const EditorActionSink *sink) {
  char item_label[96];
  struct nk_rect widget_bounds;
  int enabled = 0;
  int hovered = 0;
  char tooltip[128];

  if (!ui || !ui->ctx || !label || !view_model) {
    return 0;
  }

  ui_context_menu_format_command_label(view_model, label, command, item_label,
                                       sizeof(item_label));
  enabled = editor_viewmodel_command_available(view_model, command);
  if (!enabled) {
    nk_widget_disable_begin(ui->ctx);
  }
  widget_bounds = nk_widget_bounds(ui->ctx);
  if (!nk_button_label(ui->ctx, item_label)) {
    hovered = nk_input_is_mouse_hovering_rect(&ui->ctx->input, widget_bounds);
    if (hovered) {
      snprintf(tooltip,
               sizeof(tooltip),
               "%s%s%s",
               label,
               editor_viewmodel_command_unavailable_reason(view_model, command)[0] ? "\nUnavailable: " : "",
               editor_viewmodel_command_unavailable_reason(view_model, command));
      if (tooltip[0] != '\0') {
        nk_tooltip(ui->ctx, tooltip);
      }
    }
    if (!enabled) {
      nk_widget_disable_end(ui->ctx);
    }
    return 0;
  }
  if (!enabled) {
    nk_widget_disable_end(ui->ctx);
    return 0;
  }

  {
    EditorAction action = editor_action_make_execute_command(command);
    editor_action_emit(sink, &action);
  }
  return 1;
}

static int ui_context_menu_tool_item(UiSystem *ui,
                                     const EditorToolView *tool_view,
                                     const EditorActionSink *sink) {
  char item_label[96];
  struct nk_rect widget_bounds;
  int hovered = 0;

  if (!ui || !ui->ctx || !tool_view || !tool_view->name[0]) {
    return 0;
  }

  ui_context_menu_format_command_label(NULL,
                                       tool_view->name,
                                       EDITOR_COMMAND_NONE,
                                       item_label,
                                       sizeof(item_label));
  if (tool_view->shortcut[0] != '\0') {
    size_t name_len = strlen(tool_view->name);
    size_t shortcut_len = strlen(tool_view->shortcut);
    if (name_len + shortcut_len + 2u < sizeof(item_label)) {
      memcpy(item_label, tool_view->name, name_len);
      item_label[name_len] = ' ';
      memcpy(item_label + name_len + 1u, tool_view->shortcut, shortcut_len + 1u);
    }
  }
  if (!tool_view->available) {
    nk_widget_disable_begin(ui->ctx);
  }
  widget_bounds = nk_widget_bounds(ui->ctx);
  if (!nk_button_label(ui->ctx, item_label)) {
    hovered = nk_input_is_mouse_hovering_rect(&ui->ctx->input, widget_bounds);
    if (hovered && tool_view->tooltip[0] != '\0') {
      nk_tooltip(ui->ctx, tool_view->tooltip);
    }
    if (!tool_view->available) {
      nk_widget_disable_end(ui->ctx);
    }
    return 0;
  }
  if (!tool_view->available) {
    nk_widget_disable_end(ui->ctx);
    return 0;
  }

  {
    EditorAction action = editor_action_make_set_tool(tool_view->id);
    editor_action_emit(sink, &action);
  }
  return 1;
}

static void ui_context_menu_separator(UiSystem *ui) {
  if (!ui || !ui->ctx) {
    return;
  }

  nk_layout_row_dynamic(ui->ctx, 8.0f, 1);
  nk_spacing(ui->ctx, 1);
}

static int ui_context_menu_build_tools(UiSystem *ui,
                                       const EditorViewModel *view_model,
                                       const EditorActionSink *sink) {
  int tool_count = view_model ? view_model->tool_count : 0;
  int i = 0;

  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  for (i = 0; i < tool_count; ++i) {
    const EditorToolView *tool_view = &view_model->tools[i];

    if (!tool_view->id[0]) {
      continue;
    }

    if (tool_view->active) {
      char active_label[96];
      ui_context_menu_format_command_label(view_model,
                                           tool_view->name,
                                           tool_view->command,
                                           active_label,
                                           sizeof(active_label));
      nk_label(ui->ctx, active_label, NK_TEXT_LEFT);
    } else if (ui_context_menu_tool_item(ui, tool_view, sink)) {
      return 1;
    }
  }

  ui_context_menu_separator(ui);
  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  if (ui_context_menu_item(ui, "Zoom to Fit",
                           EDITOR_COMMAND_VIEW_ZOOM_FIT,
                           view_model,
                           sink)) {
    return 1;
  }

  ui_context_menu_separator(ui);
  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  if (editor_viewmodel_command_available(view_model,
                                         EDITOR_COMMAND_VIEW_TOGGLE_GRID)) {
    if (ui_context_menu_item(ui, "Toggle Grid",
                             EDITOR_COMMAND_VIEW_TOGGLE_GRID,
                             view_model,
                             sink)) {
      return 1;
    }
  }

  return 0;
}

static int ui_context_menu_build_selection(UiSystem *ui,
                                           const EditorViewModel *view_model,
                                           const EditorActionSink *sink) {
  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  if (ui_context_menu_item(ui, "Copy", EDITOR_COMMAND_EDIT_COPY,
                           view_model, sink)) {
    return 1;
  }
  if (ui_context_menu_item(ui, "Paste", EDITOR_COMMAND_EDIT_PASTE,
                           view_model, sink)) {
    return 1;
  }
  if (ui_context_menu_item(ui, "Delete", EDITOR_COMMAND_EDIT_DELETE,
                           view_model, sink)) {
    return 1;
  }

  return 0;
}

void ui_context_menu_reset(UiSystem *ui) {
  if (!ui) {
    return;
  }

  ui->context_menu.open = 0;
  ui->context_menu.close_requested = 0;
  ui->context_menu.mode = UI_CONTEXT_MENU_MODE_TOOLS;
  ui->context_menu.anchor_screen = vec2_make(0.0f, 0.0f);
  ui->context_menu.canvas_bounds = (RectF){0.0f, 0.0f, 0.0f, 0.0f};
  ui->context_menu.popup_bounds = (RectF){0.0f, 0.0f, 0.0f, 0.0f};
}

int ui_context_menu_is_open(const UiSystem *ui) {
  return ui && ui->context_menu.open;
}

void ui_context_menu_build(UiSystem *ui,
                           const EditorViewModel *view_model,
                           const EditorActionSink *sink) {
  RectF canvas_bounds;
  RectF host_bounds;
  RectF popup_bounds;
  struct nk_rect nk_host_bounds;
  struct nk_rect nk_popup_bounds;
  int is_open = 0;
  int menu_action_triggered = 0;

  if (!ui || !ui->ctx || !view_model || !ui->context_menu.open) {
    return;
  }

  canvas_bounds = ui->context_menu.canvas_bounds;
  if (canvas_bounds.w <= 0.0f || canvas_bounds.h <= 0.0f) {
    ui_context_menu_reset(ui);
    return;
  }

  popup_bounds = ui_context_menu_popup_bounds(&ui->context_menu);
  ui->context_menu.popup_bounds = popup_bounds;

  host_bounds.x = popup_bounds.x;
  host_bounds.y = popup_bounds.y;
  host_bounds.w = UI_CONTEXT_MENU_ANCHOR_SIZE;
  host_bounds.h = UI_CONTEXT_MENU_ANCHOR_SIZE;

  /* Popup APIs need an active Nuklear window/panel. Keep the host window to a
   * single pixel so the menu does not visually cover the canvas. */
  nk_host_bounds =
      nk_rect(host_bounds.x, host_bounds.y, host_bounds.w, host_bounds.h);
  nk_popup_bounds =
      nk_rect(0.0f, 0.0f, popup_bounds.w, popup_bounds.h);

  if (nk_begin(ui->ctx, "Canvas Context Host", nk_host_bounds,
               NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND |
                   NK_WINDOW_NO_INPUT)) {
    is_open = nk_popup_begin(ui->ctx, NK_POPUP_DYNAMIC, "Canvas Context Menu",
                             NK_WINDOW_NO_SCROLLBAR, nk_popup_bounds);
    if (is_open) {
      if (ui->context_menu.close_requested) {
        nk_popup_close(ui->ctx);
      } else if (ui->context_menu.mode == UI_CONTEXT_MENU_MODE_SELECTION) {
        menu_action_triggered = ui_context_menu_build_selection(ui, view_model, sink);
      } else {
        menu_action_triggered = ui_context_menu_build_tools(ui, view_model, sink);
      }

      if (ui->context_menu.close_requested || menu_action_triggered) {
        nk_popup_close(ui->ctx);
      }
      nk_popup_end(ui->ctx);
    }
  }
  nk_end(ui->ctx);

  if (!is_open || ui->context_menu.close_requested || menu_action_triggered) {
    ui_context_menu_reset(ui);
  }
}

int ui_context_menu_handle_key(UiSystem *ui, int key, int action) {
  if (!ui || action != GLFW_PRESS) {
    return 0;
  }

  if (key == GLFW_KEY_ESCAPE && ui->context_menu.open) {
    ui->context_menu.close_requested = 1;
    return 1;
  }

  return 0;
}

int ui_context_menu_handle_mouse_button(UiSystem *ui, Vec2 screen_pos, int button,
                                        int action) {
  if (!ui || action != GLFW_PRESS || ui->modal_active) {
    return 0;
  }

  if (ui->context_menu.open && button == GLFW_MOUSE_BUTTON_LEFT) {
    /* Match common desktop behavior: clicking blank canvas dismisses the
     * context menu immediately instead of waiting for popup internals. */
    if (!rectf_contains_point(&ui->context_menu.popup_bounds, screen_pos)) {
      ui_context_menu_reset(ui);
      return 1;
    }
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    /* Open the canvas menu from the input path so tools never see the same
     * right-click as a drawing gesture. */
    ui_context_menu_open(ui, screen_pos);
    return ui->context_menu.open;
  }

  return 0;
}
