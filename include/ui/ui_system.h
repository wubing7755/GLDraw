#ifndef GLDRAW_UI_UI_SYSTEM_H
#define GLDRAW_UI_UI_SYSTEM_H

#include <base/types.h>
#include <platform/window.h>

struct Workspace;

typedef struct UiSystem UiSystem;

UiSystem* ui_system_create(PlatformWindow* window);
void ui_system_destroy(UiSystem* ui);
void ui_system_begin_frame(UiSystem* ui);
void ui_system_build(UiSystem* ui, struct Workspace* workspace);
void ui_system_render(UiSystem* ui);
int ui_system_has_active_interaction(const UiSystem* ui);
int ui_system_blocks_pointer(const UiSystem* ui, Vec2 screen_pos);
RectF ui_system_content_bounds(const UiSystem* ui);
Color ui_system_canvas_background(const UiSystem* ui);

#endif /* GLDRAW_UI_UI_SYSTEM_H */
