/**
 * @file ui_system_internal.h
 * @brief Private UI system state shared by UI implementation modules.
 */
#ifndef GLDRAW_UI_UI_SYSTEM_INTERNAL_H
#define GLDRAW_UI_UI_SYSTEM_INTERNAL_H

#ifndef NK_NUKLEAR_H_
#ifndef NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_FIXED_TYPES
#endif
#ifndef NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_IO
#endif
#ifndef NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_VARARGS
#endif
#ifndef NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#endif
#ifndef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#endif
#ifndef NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_FONT_BAKING
#endif
#ifndef NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_DEFAULT_FONT
#endif
#include "nuklear/nuklear.h"
#endif

#ifndef NK_GLFW_GL3_H_
#include "nuklear/nuklear_glfw_gl3.h"
#endif

#include <app/workspace_layout_types.h>
#include <base/path_utils.h>
#include <base/types.h>
#include <platform/window.h>
#include <ui/editor_action.h>
#include <ui/ui_menubar.h>
#include <ui/ui_theme.h>

#define UI_THEME_ID_CAPACITY 64
#define UI_THEME_DESCRIPTOR_CACHE_MAX 64
#define UI_THEME_SETTINGS_PATH "gldraw.settings.json"
#define UI_THEME_DIRECTORY_PATH "themes"
#define UI_THEME_WATCH_INTERVAL_SECONDS 0.35

typedef enum UiContextMenuMode {
  UI_CONTEXT_MENU_MODE_TOOLS = 0,
  UI_CONTEXT_MENU_MODE_SELECTION
} UiContextMenuMode;

typedef enum UiThemeReloadReason {
  UI_THEME_RELOAD_REASON_STARTUP = 0,
  UI_THEME_RELOAD_REASON_AUTO,
  UI_THEME_RELOAD_REASON_MANUAL
} UiThemeReloadReason;

typedef struct UiContextMenuState {
  int open;
  int close_requested;
  UiContextMenuMode mode;
  Vec2 anchor_screen;
  RectF canvas_bounds;
  RectF popup_bounds;
} UiContextMenuState;

typedef struct UiSystem UiSystem;

struct UiSystem {
  struct nk_glfw glfw;
  struct nk_context *ctx;
  PlatformWindow *window;
  UiMenuBar *menu_bar;
  UiThemeTokens theme;
  char active_theme_id[UI_THEME_ID_CAPACITY];
  char theme_settings_path[GLDRAW_PATH_MAX];
  UiThemeDescriptor theme_descriptors_cache[UI_THEME_DESCRIPTOR_CACHE_MAX];
  unsigned long long theme_directory_signature;
  double theme_watch_last_check_seconds;
  RectF appbar_bounds;
  RectF rail_bounds;
  RectF panel_bounds;
  RectF status_bounds;
  RectF content_bounds;
  WorkspaceLayout layout_snapshot;
  EditorActionSink action_sink;
  float inspector_anim_t;
  int inspector_target_visible;
  int inspector_anim_initialized;
  int last_selection_count;
  LayerId editing_layer_id;
  char layer_name_buffer[EDITOR_VIEWMODEL_NAME_CAPACITY];
  UiDialogKind dialog_kind_snapshot;
  char dialog_input_buffer[EDITOR_ACTION_TEXT_CAPACITY];
  int modal_active;
  UiContextMenuState context_menu;
  double last_frame_seconds;
};

void ui_system_emit_action(UiSystem *ui, const EditorAction *action);
void ui_system_emit_status(UiSystem *ui, const char *fmt, ...);
void ui_system_sync_menubar_themes(UiSystem *ui);
int ui_system_set_theme(UiSystem *ui, const char *theme_id, int persist_selection);
void ui_system_reload_themes(UiSystem *ui,
                             int notify_status,
                             UiThemeReloadReason reason);
void ui_system_load_theme_from_settings(UiSystem *ui);
void ui_system_poll_theme_hot_reload(UiSystem *ui, double now_seconds);
float ui_clampf(float value, float min_value, float max_value);
float ui_smoothstep(float t);
void ui_publish_layout(UiSystem *ui, int width, int height);
void ui_modal_dialogs(UiSystem *ui,
                      const EditorViewModel *view_model,
                      int window_width,
                      int window_height);
void ui_tool_rail(UiSystem *ui,
                  const EditorViewModel *view_model,
                  RectF bounds);
void ui_status_bar(UiSystem *ui,
                   const EditorViewModel *view_model,
                   int window_width,
                   int window_height);
void ui_selection_panel(UiSystem *ui,
                        const EditorViewModel *view_model,
                        RectF bounds);

#endif /* GLDRAW_UI_UI_SYSTEM_INTERNAL_H */
