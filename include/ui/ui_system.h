/**
 * @file ui_system.h
 * @brief UI system lifecycle and layout query interface.
 */
#ifndef GLDRAW_UI_UI_SYSTEM_H
#define GLDRAW_UI_UI_SYSTEM_H

#include <base/types.h>
#include <platform/window.h>
#include <ui/editor_action.h>
#include <ui/editor_viewmodel.h>

typedef struct UiSystem UiSystem;

/**
 * @brief Create a UI system instance.
 * @param window Already-initialized platform window.
 * @return `UiSystem*` on success, `NULL` on failure.
 */
UiSystem* ui_system_create(PlatformWindow* window);

/**
 * @brief Destroy the UI system and release resources.
 * @param ui UI system instance.
 * @return No return value.
 */
void ui_system_destroy(UiSystem* ui);

/**
 * @brief Begin a UI frame.
 * @param ui UI system instance.
 * @return No return value.
 */
void ui_system_begin_frame(UiSystem* ui);

/**
 * @brief Assign the action sink used to dispatch UI-originated editor actions.
 * @param ui UI system instance.
 * @param sink Action sink copied into UI state.
 * @return No return value.
 */
void ui_system_set_action_sink(UiSystem* ui, const EditorActionSink* sink);

/**
 * @brief Build the entire frame UI from a read-only editor view model.
 * @param ui UI system instance.
 * @param view_model Read-only editor view model.
 * @return No return value.
 */
void ui_system_build(UiSystem* ui, const EditorViewModel* view_model);

/**
 * @brief Offer one key event to the UI before routing it elsewhere.
 * @param ui UI system instance.
 * @param key GLFW key code.
 * @param action GLFW action code.
 * @return Non-zero if the UI consumed the event, zero otherwise.
 */
int ui_system_handle_key(UiSystem* ui, int key, int action);

/**
 * @brief Offer one mouse button event to the UI before routing it elsewhere.
 * @param ui UI system instance.
 * @param screen_pos Pointer position in screen coordinates.
 * @param button GLFW mouse button code.
 * @param action GLFW action code.
 * @return Non-zero if the UI consumed the event, zero otherwise.
 */
int ui_system_handle_mouse_button(UiSystem* ui,
                                  Vec2 screen_pos,
                                  int button,
                                  int action);

/**
 * @brief Commit UI draw commands.
 * @param ui UI system instance.
 * @return No return value.
 */
void ui_system_render(UiSystem* ui);

/**
 * @brief Query whether the UI has an ongoing interaction.
 * @param ui UI system instance.
 * @return Non-zero if an interaction is in progress, zero otherwise.
 */
int ui_system_has_active_interaction(const UiSystem* ui);

/**
 * @brief Check whether the screen point is intercepted by the UI region.
 * @param ui UI system instance.
 * @param screen_pos Screen coordinate point.
 * @return Non-zero if intercepted by the UI region, zero otherwise.
 */
int ui_system_blocks_pointer(const UiSystem* ui, Vec2 screen_pos);

/**
 * @brief Get window hit-test bounds.
 * @param ui UI system instance.
 * @return Window boundary rectangle.
 */
RectF ui_system_window_bounds(const UiSystem* ui);

/**
 * @brief Get canvas content area bounds.
 * @param ui UI system instance.
 * @return Content area rectangle.
 */
RectF ui_system_content_bounds(const UiSystem* ui);

/**
 * @brief Check whether a point lies inside the canvas content area.
 * @param ui UI system instance.
 * @param screen_pos Screen coordinate point.
 * @return Non-zero if inside the canvas region, zero otherwise.
 */
int ui_system_point_in_canvas(const UiSystem* ui, Vec2 screen_pos);

/**
 * @brief Get the canvas background color for the current theme.
 * @param ui UI system instance.
 * @return Canvas background color.
 */
Color ui_system_canvas_background(const UiSystem* ui);

/**
 * @brief Get the latest full layout snapshot published by the UI.
 * @param ui UI system instance.
 * @return Current layout snapshot.
 */
WorkspaceLayout ui_system_layout(const UiSystem* ui);

#endif /* GLDRAW_UI_UI_SYSTEM_H */
