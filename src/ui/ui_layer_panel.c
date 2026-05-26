/**
 * @file ui_layer_panel.c
 * @brief Layer list and active-layer controls for the inspector panel.
 */
#include "ui/ui_system_internal.h"

#include <stdio.h>
#include <string.h>

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

  if (!ui || !layer_view || ui->modal_active ||
      ui->layer_name_buffer[0] == '\0' ||
      strcmp(ui->layer_name_buffer, layer_view->name) == 0) {
    return;
  }

  action = editor_action_make_rename_layer(layer_view->id, ui->layer_name_buffer);
  ui_system_emit_action(ui, &action);
}

void ui_layers_panel(UiSystem *ui, struct nk_context *ctx,
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
