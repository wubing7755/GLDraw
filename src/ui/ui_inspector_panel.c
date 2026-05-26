/**
 * @file ui_inspector_panel.c
 * @brief UiSystem inspector, property, and layer panel rendering.
 */
#include "ui/ui_system_internal.h"

#include <math.h>
#include <stdio.h>

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

void ui_selection_panel(UiSystem *ui, const EditorViewModel *view_model,
                        RectF bounds) {
  struct nk_context *ctx = ui->ctx;

  ui->panel_bounds = bounds;
  if (!ctx || !view_model || bounds.w <= 0.0f || bounds.h <= 0.0f) {
    return;
  }

  if (nk_begin(ctx, "Inspector", nk_rect(bounds.x, bounds.y, bounds.w, bounds.h),
               NK_WINDOW_BORDER | NK_WINDOW_TITLE |
                   NK_WINDOW_SCROLL_AUTO_HIDE)) {
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
