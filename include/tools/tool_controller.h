/**
 * @file tool_controller.h
 * @brief Active tool management and input dispatch interface.
 */
#ifndef GLDRAW_TOOLS_TOOL_CONTROLLER_H
#define GLDRAW_TOOLS_TOOL_CONTROLLER_H

#include <tools/tool.h>

/**
 * @struct ToolController
 * @brief Tool controller state.
 *
 * @member tools Tool instance array.
 * @member active_kind Currently active tool type.
 * @member pointer_captured Whether the pointer is captured by a tool.
 * @member last_screen Most recent screen coordinates.
 * @member last_world Most recent world coordinates.
 */
typedef struct {
    Tool tools[TOOL_KIND_COUNT];
    ToolKind active_kind;
    int pointer_captured;
    Vec2 last_screen;
    Vec2 last_world;
} ToolController;

/**
 * @brief Initialize the tool controller.
 * @param controller Controller instance.
 * @return No return value.
 */
void tool_controller_init(ToolController* controller);

/**
 * @brief Shut down the tool controller and release tool state.
 * @param controller Controller instance.
 * @return No return value.
 */
void tool_controller_shutdown(ToolController* controller);

/**
 * @brief Switch the currently active tool.
 * @param controller Controller instance.
 * @param context Tool context.
 * @param kind Target tool type.
 * @return No return value.
 */
void tool_controller_set_active(ToolController* controller, ToolContext* context, ToolKind kind);

/**
 * @brief Get the currently active tool.
 * @param controller Controller instance.
 * @return Pointer to the active tool; returns `NULL` if parameters are invalid.
 */
Tool* tool_controller_get_active(ToolController* controller);

/**
 * @brief Get the label text of the currently active tool.
 * @param controller Controller instance.
 * @return Tool label string.
 */
const char* tool_controller_active_label(const ToolController* controller);

/**
 * @brief Get the overlay preview object of the currently active tool.
 * @param controller Controller instance.
 * @return Overlay object pointer; returns `NULL` if no preview is available.
 */
GraphicObject* tool_controller_overlay_object(const ToolController* controller);

/**
 * @brief Dispatch a mouse pointer-down event.
 * @param controller Controller instance.
 * @param context Tool context.
 * @param event Input event.
 * @return No return value.
 */
void tool_controller_pointer_down(ToolController* controller, ToolContext* context, const ToolEvent* event);

/**
 * @brief Dispatch a mouse pointer-move event.
 * @param controller Controller instance.
 * @param context Tool context.
 * @param event Input event.
 * @return No return value.
 */
void tool_controller_pointer_move(ToolController* controller, ToolContext* context, const ToolEvent* event);

/**
 * @brief Dispatch a mouse pointer-up event.
 * @param controller Controller instance.
 * @param context Tool context.
 * @param event Input event.
 * @return No return value.
 */
void tool_controller_pointer_up(ToolController* controller, ToolContext* context, const ToolEvent* event);

/**
 * @brief Dispatch a key-down event.
 * @param controller Controller instance.
 * @param context Tool context.
 * @param key GLFW key value.
 * @param mods Modifier key mask.
 * @return No return value.
 */
void tool_controller_key_down(ToolController* controller, ToolContext* context, int key, int mods);

/**
 * @brief Handle scroll-wheel zoom events.
 * @param controller Controller instance.
 * @param context Tool context.
 * @param screen_pos Current screen coordinate anchor.
 * @param yoffset Scroll wheel Y delta.
 * @return No return value.
 */
void tool_controller_scroll(ToolController* controller, ToolContext* context, Vec2 screen_pos, float yoffset);

#endif /* GLDRAW_TOOLS_TOOL_CONTROLLER_H */
