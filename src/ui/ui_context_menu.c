/**
 * @file ui_context_menu.c
 * @brief Canvas context menu implementation.
 */
#include "ui/ui_context_menu.h"
#include "ui/ui_system_internal.h"

#include <app/command_registry.h>
#include <base/math2d.h>
#include <tools/tool_controller.h>

#include <stdio.h>

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

static ToolContext ui_context_menu_tool_context(Workspace *workspace) {
  ToolContext context;

  context.workspace = workspace;
  context.document = &workspace->core.document;
  context.history = &workspace->core.history;
  context.canvas = &workspace->core.canvas;
  context.selection = &workspace->session.selection;
  return context;
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

static void ui_context_menu_open(UiSystem *ui, Workspace *workspace,
                                 Vec2 screen_pos) {
  RectF canvas_bounds;

  if (!ui || !workspace) {
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
  ui->context_menu.mode = workspace->session.selection.count > 0
                              ? UI_CONTEXT_MENU_MODE_SELECTION
                              : UI_CONTEXT_MENU_MODE_TOOLS;
  ui->context_menu.anchor_screen = screen_pos;
  ui->context_menu.canvas_bounds = canvas_bounds;
  ui->context_menu.popup_bounds = ui_context_menu_popup_bounds(&ui->context_menu);
}

static void ui_context_menu_format_command_label(const Workspace *workspace,
                                                 const char *label,
                                                 const char *command_id,
                                                 KeyScope scope,
                                                 char *buffer,
                                                 size_t buffer_size) {
  char shortcut[64];

  if (!buffer || buffer_size == 0u) {
    return;
  }

  buffer[0] = '\0';
  if (!label) {
    return;
  }

  shortcut[0] = '\0';
  if (workspace && command_id && command_id[0] != '\0') {
    keymap_format_command_shortcut(&workspace->session.keymap, command_id, scope,
                                   shortcut, sizeof(shortcut));
  }

  if (shortcut[0] != '\0') {
    snprintf(buffer, buffer_size, "%-18s %s", label, shortcut);
  } else {
    snprintf(buffer, buffer_size, "%s", label);
  }
}

static int ui_context_menu_item(UiSystem *ui, const char *label,
                                const char *command_id, EditorCommand command,
                                Workspace *workspace) {
  char item_label[96];
  ToolContext context;

  if (!ui || !ui->ctx || !label || !workspace) {
    return 0;
  }

  ui_context_menu_format_command_label(workspace, label, command_id,
                                       KEY_SCOPE_GLOBAL, item_label,
                                       sizeof(item_label));
  if (!nk_button_label(ui->ctx, item_label)) {
    return 0;
  }

  context = ui_context_menu_tool_context(workspace);
  command_registry_execute(workspace, &context, command);
  return 1;
}

static void ui_context_menu_separator(UiSystem *ui) {
  if (!ui || !ui->ctx) {
    return;
  }

  nk_layout_row_dynamic(ui->ctx, 8.0f, 1);
  nk_spacing(ui->ctx, 1);
}

static int ui_context_menu_build_tools(UiSystem *ui, Workspace *workspace) {
  const ToolKind active_tool = workspace->core.tools.active_kind;

  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  if (active_tool == TOOL_KIND_SELECT) {
    nk_label(ui->ctx, "Select Tool (V)", NK_TEXT_LEFT);
  } else if (ui_context_menu_item(ui, "Select Tool", "tool.select",
                                  EDITOR_COMMAND_TOOL_SELECT, workspace)) {
    return 1;
  }

  if (active_tool == TOOL_KIND_PAN) {
    nk_label(ui->ctx, "Pan Tool (H)", NK_TEXT_LEFT);
  } else if (ui_context_menu_item(ui, "Pan Tool", "tool.pan",
                                  EDITOR_COMMAND_TOOL_PAN, workspace)) {
    return 1;
  }

  if (active_tool == TOOL_KIND_LINE) {
    nk_label(ui->ctx, "Line Tool (L)", NK_TEXT_LEFT);
  } else if (ui_context_menu_item(ui, "Line Tool", "tool.line",
                                  EDITOR_COMMAND_TOOL_LINE, workspace)) {
    return 1;
  }

  if (active_tool == TOOL_KIND_RECT) {
    nk_label(ui->ctx, "Rectangle Tool (R)", NK_TEXT_LEFT);
  } else if (ui_context_menu_item(ui, "Rectangle Tool", "tool.rect",
                                  EDITOR_COMMAND_TOOL_RECT, workspace)) {
    return 1;
  }

  if (active_tool == TOOL_KIND_ELLIPSE) {
    nk_label(ui->ctx, "Ellipse Tool (E)", NK_TEXT_LEFT);
  } else if (ui_context_menu_item(ui, "Ellipse Tool", "tool.ellipse",
                                  EDITOR_COMMAND_TOOL_ELLIPSE, workspace)) {
    return 1;
  }

  ui_context_menu_separator(ui);
  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  if (ui_context_menu_item(ui, "Zoom to Fit", "view.zoom_fit",
                           EDITOR_COMMAND_VIEW_ZOOM_FIT, workspace)) {
    return 1;
  }

  ui_context_menu_separator(ui);
  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  if (workspace->core.canvas.show_grid) {
    if (ui_context_menu_item(ui, "Hide Grid", "view.toggle_grid",
                             EDITOR_COMMAND_VIEW_TOGGLE_GRID, workspace)) {
      return 1;
    }
  } else if (ui_context_menu_item(ui, "Show Grid", "view.toggle_grid",
                                  EDITOR_COMMAND_VIEW_TOGGLE_GRID,
                                  workspace)) {
    return 1;
  }

  return 0;
}

static int ui_context_menu_build_selection(UiSystem *ui, Workspace *workspace) {
  nk_layout_row_dynamic(ui->ctx, ui->theme.row_height, 1);
  if (ui_context_menu_item(ui, "Copy", "edit.copy", EDITOR_COMMAND_EDIT_COPY,
                           workspace)) {
    return 1;
  }
  if (ui_context_menu_item(ui, "Paste", "edit.paste", EDITOR_COMMAND_EDIT_PASTE,
                           workspace)) {
    return 1;
  }
  if (ui_context_menu_item(ui, "Delete", "edit.delete",
                           EDITOR_COMMAND_EDIT_DELETE, workspace)) {
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

void ui_context_menu_build(UiSystem *ui, Workspace *workspace) {
  RectF canvas_bounds;
  RectF host_bounds;
  RectF popup_bounds;
  struct nk_rect nk_host_bounds;
  struct nk_rect nk_popup_bounds;
  int is_open = 0;
  int menu_action_triggered = 0;

  if (!ui || !ui->ctx || !workspace || !ui->context_menu.open) {
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
        menu_action_triggered = ui_context_menu_build_selection(ui, workspace);
      } else {
        menu_action_triggered = ui_context_menu_build_tools(ui, workspace);
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

int ui_context_menu_handle_mouse_button(UiSystem *ui, Workspace *workspace,
                                        Vec2 screen_pos, int button,
                                        int action) {
  if (!ui || !workspace || action != GLFW_PRESS || ui->modal_active) {
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
    ui_context_menu_open(ui, workspace, screen_pos);
    return ui->context_menu.open;
  }

  return 0;
}
