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

#include <app/workspace.h>
#include <base/types.h>
#include <ui/ui_menubar.h>
#include <ui/ui_theme.h>

#define UI_THEME_ID_CAPACITY 64
#define UI_THEME_DESCRIPTOR_CACHE_MAX 64

typedef enum UiContextMenuMode {
  UI_CONTEXT_MENU_MODE_TOOLS = 0,
  UI_CONTEXT_MENU_MODE_SELECTION
} UiContextMenuMode;

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
  GLFWwindow *window_handle;
  UiMenuBar *menu_bar;
  UiThemeTokens theme;
  char active_theme_id[UI_THEME_ID_CAPACITY];
  char theme_settings_path[260];
  UiThemeDescriptor theme_descriptors_cache[UI_THEME_DESCRIPTOR_CACHE_MAX];
  unsigned long long theme_directory_signature;
  double theme_watch_last_check_seconds;
  RectF appbar_bounds;
  RectF rail_bounds;
  RectF panel_bounds;
  RectF status_bounds;
  RectF content_bounds;
  WorkspaceLayout layout_snapshot;
  float inspector_anim_t;
  int inspector_target_visible;
  int inspector_anim_initialized;
  int inspector_edit_active;
  DocumentSnapshot inspector_edit_before_snapshot;
  SelectionSet inspector_edit_before_selection;
  int modal_active;
  UiContextMenuState context_menu;
  double last_frame_seconds;
};

#endif /* GLDRAW_UI_UI_SYSTEM_INTERNAL_H */
