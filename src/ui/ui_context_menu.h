/**
 * @file ui_context_menu.h
 * @brief Canvas context menu behavior for the UI system.
 */
#ifndef GLDRAW_UI_CONTEXT_MENU_H
#define GLDRAW_UI_CONTEXT_MENU_H

#include <base/types.h>
#include <ui/editor_action.h>
#include <ui/editor_viewmodel.h>

struct UiSystem;

void ui_context_menu_reset(struct UiSystem *ui);
int ui_context_menu_is_open(const struct UiSystem *ui);
void ui_context_menu_build(struct UiSystem *ui,
                           const EditorViewModel *view_model,
                           const EditorActionSink *sink);
int ui_context_menu_handle_key(struct UiSystem *ui, int key, int action);
int ui_context_menu_handle_mouse_button(struct UiSystem *ui,
                                        Vec2 screen_pos,
                                        int button,
                                        int action);

#endif /* GLDRAW_UI_CONTEXT_MENU_H */
