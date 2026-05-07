/**
 * @file application_runtime.h
 * @brief Application runtime coordination helpers.
 */
#ifndef GLDRAW_APP_APPLICATION_RUNTIME_H
#define GLDRAW_APP_APPLICATION_RUNTIME_H

#include <tools/tool.h>

typedef struct Application Application;

ToolContext application_runtime_tool_context(Application* app);
int application_runtime_pointer_blocks_canvas(const Application* app, Vec2 screen_pos);
void application_runtime_sync_tool_pointer_anchor(Application* app);
void application_runtime_sync_canvas_boundary(Application* app);
void application_runtime_update_canvas_viewport(Application* app);
ToolEvent application_runtime_make_tool_event(Application* app,
                                              int button,
                                              int mods,
                                              float wheel_y);

#endif /* GLDRAW_APP_APPLICATION_RUNTIME_H */
