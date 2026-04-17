/**
 * @file ui_system.h
 * @brief Nuklear UI system lifecycle and layout query API.
 *
 * Role in project:
 * - Builds toolbar/menu/inspector/status UI each frame.
 * - Publishes UI layout bounds used by app input filtering and canvas viewport.
 *
 * Module relationships:
 * - Created/destroyed by application runtime.
 * - Reads/writes workspace state and consumes platform window context.
 */
#ifndef GLDRAW_UI_UI_SYSTEM_H
#define GLDRAW_UI_UI_SYSTEM_H

#include <base/types.h>
#include <platform/window.h>

struct Workspace;

typedef struct UiSystem UiSystem;

/** Create UI system for an initialized window/context. Returns `NULL` on failure. */
UiSystem* ui_system_create(PlatformWindow* window);
/** Destroy UI resources and contexts. */
void ui_system_destroy(UiSystem* ui);
/** Begin a new UI frame (input capture/reset). */
void ui_system_begin_frame(UiSystem* ui);
/** Build all UI panels and apply workspace mutations. */
void ui_system_build(UiSystem* ui, struct Workspace* workspace);
/** Render prepared UI draw commands. */
void ui_system_render(UiSystem* ui);
/** Return non-zero when any UI widget is actively interacting. */
int ui_system_has_active_interaction(const UiSystem* ui);
/** Return non-zero when a screen point is over UI-occupied regions. */
int ui_system_blocks_pointer(const UiSystem* ui, Vec2 screen_pos);
/** Get clamped UI-published window bounds used for hit testing and viewport fallback. */
RectF ui_system_window_bounds(const UiSystem* ui);
/** Get computed content area available for canvas. */
RectF ui_system_content_bounds(const UiSystem* ui);
/** Return non-zero when screen point lies inside canvas content region. */
int ui_system_point_in_canvas(const UiSystem* ui, Vec2 screen_pos);
/** Get canvas background color derived from active theme. */
Color ui_system_canvas_background(const UiSystem* ui);

#endif /* GLDRAW_UI_UI_SYSTEM_H */
