/**
 * @file tool_controller.h
 * @brief Active tool routing and input dispatch API.
 *
 * Role in project:
 * - Stores all tool instances and tracks the current active tool.
 * - Forwards pointer/key/scroll events to the active tool implementation.
 *
 * Module relationships:
 * - Built on `tool.h` abstractions.
 * - Called by application event callbacks and UI tool selectors.
 */
#ifndef GLDRAW_TOOLS_TOOL_CONTROLLER_H
#define GLDRAW_TOOLS_TOOL_CONTROLLER_H

#include <tools/tool.h>

typedef struct {
    Tool tools[TOOL_KIND_COUNT];
    ToolKind active_kind;
    int pointer_captured;
    Vec2 last_screen;
    Vec2 last_world;
} ToolController;

/** Initialize all tool slots and internal state. Complexity: `O(tool_count)`. */
void tool_controller_init(ToolController* controller);
/** Shutdown tools and free per-tool state. Complexity: `O(tool_count)`. */
void tool_controller_shutdown(ToolController* controller);

/** Switch active tool and run deactivate/activate hooks as needed. Complexity: `O(1)`. */
void tool_controller_set_active(ToolController* controller, ToolContext* context, ToolKind kind);
/** Get mutable pointer to currently active tool, or `NULL` if controller invalid. */
Tool* tool_controller_get_active(ToolController* controller);
/** Get active tool display label. */
const char* tool_controller_active_label(const ToolController* controller);
/** Get overlay object owned by active tool (preview rendering). */
GraphicObject* tool_controller_overlay_object(const ToolController* controller);

/** Dispatch pointer press and capture pointer ownership. */
void tool_controller_pointer_down(ToolController* controller, ToolContext* context, const ToolEvent* event);
/** Dispatch pointer move to active tool. */
void tool_controller_pointer_move(ToolController* controller, ToolContext* context, const ToolEvent* event);
/** Dispatch pointer release and release pointer capture. */
void tool_controller_pointer_up(ToolController* controller, ToolContext* context, const ToolEvent* event);
/** Dispatch keyboard input to global shortcuts or active tool. */
void tool_controller_key_down(ToolController* controller, ToolContext* context, int key, int mods);
/** Handle mouse wheel scroll for zoom around a screen anchor. */
void tool_controller_scroll(ToolController* controller, ToolContext* context, Vec2 screen_pos, float yoffset);

#endif /* GLDRAW_TOOLS_TOOL_CONTROLLER_H */
