/**
 * @file ui_theme_runtime.c
 * @brief UiSystem theme selection, reload, and hot-reload runtime.
 */
#include "ui/ui_system_internal.h"

#include <ui/ui_theme.h>

#include <stdio.h>
#include <string.h>

static const char *ui_theme_reload_reason_label(UiThemeReloadReason reason) {
  switch (reason) {
  case UI_THEME_RELOAD_REASON_MANUAL:
    return "manually";
  case UI_THEME_RELOAD_REASON_AUTO:
    return "auto";
  case UI_THEME_RELOAD_REASON_STARTUP:
  default:
    return "startup";
  }
}

static const char *ui_system_theme_directory_path(const UiSystem *ui) {
  if (ui && ui->theme_directory_path[0] != '\0') {
    return ui->theme_directory_path;
  }
  return UI_THEME_DIRECTORY_PATH;
}

void ui_system_sync_menubar_themes(UiSystem *ui) {
  int theme_count = 0;
  int active_theme_index = 0;

  if (!ui || !ui->menu_bar) {
    return;
  }

  theme_count = ui_theme_count();
  if (theme_count < 0) {
    theme_count = 0;
  }
  if (theme_count > UI_THEME_DESCRIPTOR_CACHE_MAX) {
    theme_count = UI_THEME_DESCRIPTOR_CACHE_MAX;
  }

  for (int i = 0; i < theme_count; ++i) {
    const UiThemeDescriptor *descriptor = ui_theme_descriptor_at(i);
    ui->theme_descriptors_cache[i].id = descriptor ? descriptor->id : "";
    ui->theme_descriptors_cache[i].label = descriptor ? descriptor->label : "";
  }

  ui_menubar_set_themes(ui->menu_bar, ui->theme_descriptors_cache, theme_count);

  active_theme_index = ui_theme_index_of_id(ui->active_theme_id);
  if (active_theme_index < 0 || active_theme_index >= theme_count) {
    active_theme_index = ui_theme_index_of_id(ui_theme_default_id());
  }
  if (active_theme_index < 0) {
    active_theme_index = 0;
  }
  ui_menubar_set_active_theme_index(ui->menu_bar, active_theme_index);
}

int ui_system_set_theme(UiSystem *ui, const char *theme_id,
                        int persist_selection) {
  int theme_index = -1;
  const UiThemeDescriptor *descriptor = NULL;

  if (!ui || !ui->ctx) {
    return 0;
  }

  theme_index = ui_theme_index_of_id(theme_id);
  if (theme_index < 0) {
    theme_index = ui_theme_index_of_id(ui_theme_default_id());
  }
  descriptor = ui_theme_descriptor_at(theme_index);
  if (!descriptor || !descriptor->id) {
    return 0;
  }

  snprintf(ui->active_theme_id, sizeof(ui->active_theme_id), "%s",
           descriptor->id);

  ui->theme = ui_theme_tokens_for_id(ui->active_theme_id);
  ui_theme_apply(ui->ctx, &ui->theme);

  if (ui->menu_bar) {
    ui_menubar_set_height(ui->menu_bar, ui->theme.menu_height);
    ui_system_sync_menubar_themes(ui);
  }

  if (persist_selection) {
    ui_theme_save_selected_id(ui->theme_settings_path, ui->active_theme_id);
  }

  return 1;
}

void ui_system_reload_themes(UiSystem *ui, int notify_status,
                             UiThemeReloadReason reason) {
  char previous_theme_id[UI_THEME_ID_CAPACITY];
  int custom_theme_count = 0;
  int fallback_to_default = 0;

  if (!ui) {
    return;
  }

  snprintf(previous_theme_id, sizeof(previous_theme_id), "%s",
           ui->active_theme_id);
  custom_theme_count = ui_theme_reload_external(ui_system_theme_directory_path(ui));
  if (custom_theme_count < 0) {
    ui->theme_directory_signature =
        ui_theme_external_signature(ui_system_theme_directory_path(ui));
    if (notify_status) {
      const char *error_summary = ui_theme_last_reload_error();
      ui_system_emit_status(ui,
                            "Theme reload failed: %s",
                            (error_summary && error_summary[0] != '\0')
                                ? error_summary
                                : "invalid theme file");
    }
    return;
  }

  ui->theme_directory_signature =
      ui_theme_external_signature(ui_system_theme_directory_path(ui));
  ui_system_set_theme(ui, previous_theme_id, 0);
  fallback_to_default = (strcmp(previous_theme_id, ui->active_theme_id) != 0);

  if (notify_status) {
    const char *reason_label = ui_theme_reload_reason_label(reason);
    if (fallback_to_default) {
      ui_system_emit_status(
          ui,
          "Themes %s reloaded (%d custom), active theme missing -> fallback",
          reason_label, custom_theme_count);
    } else {
      ui_system_emit_status(ui, "Themes %s reloaded (%d custom)", reason_label,
                            custom_theme_count);
    }
  }
}

void ui_system_load_theme_from_settings(UiSystem *ui) {
  char loaded_theme_id[UI_THEME_ID_CAPACITY];

  if (!ui) {
    return;
  }

  if (ui_theme_load_selected_id(ui->theme_settings_path, loaded_theme_id,
                                sizeof(loaded_theme_id))) {
    if (ui_system_set_theme(ui, loaded_theme_id, 0)) {
      return;
    }
  }

  ui_system_set_theme(ui, ui_theme_default_id(), 0);
}

void ui_system_poll_theme_hot_reload(UiSystem *ui, double now_seconds) {
  unsigned long long signature = 0ull;

  if (!ui) {
    return;
  }

  if ((now_seconds - ui->theme_watch_last_check_seconds) <
      UI_THEME_WATCH_INTERVAL_SECONDS) {
    return;
  }
  ui->theme_watch_last_check_seconds = now_seconds;

  signature = ui_theme_external_signature(ui_system_theme_directory_path(ui));
  if (signature == ui->theme_directory_signature) {
    return;
  }

  ui_system_reload_themes(ui, 1, UI_THEME_RELOAD_REASON_AUTO);
}
