/**
 * @file ui_context_menu_render.c
 * @brief Canvas context menu item and popup rendering.
 */
#include "ui_context_menu_internal.h"

#include <app/command_types.h>

#include <stdio.h>
#include <string.h>

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
  int enabled = 0;

  if (!ui || !ui->ctx || !label || !view_model) {
    return 0;
  }

  ui_context_menu_format_command_label(view_model, label, command, item_label,
                                       sizeof(item_label));
  enabled = editor_viewmodel_command_available(view_model, command);
  if (!enabled) {
    nk_widget_disable_begin(ui->ctx);
  }
  if (!nk_button_label(ui->ctx, item_label)) {
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
  if (!nk_button_label(ui->ctx, item_label)) {
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
