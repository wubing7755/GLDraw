/**
 * @file ui_layout.c
 * @brief UiSystem layout interpolation and published layout snapshot helpers.
 */
#include "ui/ui_system_internal.h"

#include <math.h>

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

float ui_clampf(float value, float min_value, float max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

float ui_smoothstep(float t) {
  float clamped = ui_clampf(t, 0.0f, 1.0f);
  return clamped * clamped * (3.0f - 2.0f * clamped);
}

void ui_publish_layout(UiSystem *ui, int width, int height) {
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
