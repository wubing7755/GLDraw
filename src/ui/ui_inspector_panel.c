/**
 * @file ui_inspector_panel.c
 * @brief UiSystem inspector, property, and layer panel rendering.
 */
#include "ui/ui_system_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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

static const EditorLayerView *ui_find_active_layer_view(
    const EditorViewModel *view_model) {
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

static void ui_sync_layer_name_edit(UiSystem *ui,
                                    const EditorLayerView *layer_view) {
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

static void ui_submit_layer_rename(UiSystem *ui,
                                   const EditorLayerView *layer_view) {
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
    const EditorLayerView *layer_view = &view_model->layers[i];
    char label[96];
    char count_label[24];
    const char *eye = layer_view->visible ? "Hide" : "Show";
    const char *lock = layer_view->locked ? "Unlock" : "Lock";

    snprintf(label, sizeof(label), "%s%s", layer_view->active ? "* " : "",
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
