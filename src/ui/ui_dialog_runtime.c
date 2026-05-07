/**
 * @file ui_dialog_runtime.c
 * @brief UiSystem dialog state coordination and modal dialog action emission.
 */
#include "ui/ui_system_internal.h"

#include <ui/ui_dialog.h>

#include <stdio.h>

void ui_modal_dialogs(UiSystem *ui, const EditorViewModel *view_model,
                      int window_width, int window_height) {
  UiDialogState dialog;
  UiDialogResult result = UI_DIALOG_RESULT_NONE;

  if (!ui || !view_model || view_model->active_dialog.kind == UI_DIALOG_NONE ||
      !ui->ctx) {
    return;
  }

  dialog = view_model->active_dialog;
  if (dialog.kind == UI_DIALOG_SAVE_AS) {
    if (ui->dialog_kind_snapshot != dialog.kind) {
      snprintf(ui->dialog_input_buffer, sizeof(ui->dialog_input_buffer), "%s",
               dialog.payload.text);
      ui->dialog_kind_snapshot = dialog.kind;
    }
    snprintf(dialog.payload.text, sizeof(dialog.payload.text), "%s",
             ui->dialog_input_buffer);
  } else {
    ui->dialog_kind_snapshot = dialog.kind;
  }

  result = ui_dialog_show(ui->ctx, &dialog, window_width, window_height);
  if (dialog.kind == UI_DIALOG_SAVE_AS) {
    snprintf(ui->dialog_input_buffer, sizeof(ui->dialog_input_buffer), "%s",
             dialog.payload.text);
  }
  if (result != UI_DIALOG_RESULT_NONE) {
    EditorAction action =
        editor_action_make_resolve_dialog(result, dialog.payload.text);
    ui_system_emit_action(ui, &action);
  }
}
