/**
 * @file tool_internal.h
 * @brief Shared helpers for individual tool implementations.
 *
 * This header is private to src/tools/ -- it is not part of the public API.
 */
#ifndef GLDRAW_TOOLS_TOOL_INTERNAL_H
#define GLDRAW_TOOLS_TOOL_INTERNAL_H

#include <tools/tool_controller.h>

/**
 * @brief Destroy the tool's overlay object if present.
 */
void destroy_overlay(Tool* tool);

/**
 * @brief Build a RectF from two arbitrary corner points.
 */
RectF rect_from_points(Vec2 a, Vec2 b);

/**
 * @brief Allocate a tool with heap-allocated state of @p state_size bytes.
 */
int default_tool_create(Tool* tool, size_t state_size);

/**
 * @brief Destroy overlay and free tool state (NULL-safe).
 */
void default_tool_destroy(Tool* tool);

/**
 * @brief Default draw-overlay callback: returns the tool's overlay_object.
 */
GraphicObject* default_draw_overlay(Tool* tool, ToolContext* context);

#endif /* GLDRAW_TOOLS_TOOL_INTERNAL_H */
