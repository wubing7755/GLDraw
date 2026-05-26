/**
 * @file ui_context_menu.c
 * @brief Canvas context menu implementation.
 */
#include "ui_context_menu_internal.h"

#include <base/math2d.h>

static float ui_context_menu_clampf(float value, float min_value, float max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

RectF ui_context_menu_popup_bounds(const UiContextMenuState *menu) {
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
