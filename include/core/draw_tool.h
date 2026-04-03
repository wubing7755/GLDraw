#ifndef DRAW_TOOL_H
#define DRAW_TOOL_H

#include <core/tool.h>

/* =============================================================================
 * Phase 3: DrawTool — draws shapes on mouse drag
 *
 * Supports LINE, CIRCLE, RECT via shape_type field.
 * =============================================================================
 */

typedef struct DrawToolCtx DrawToolCtx;

/* Create a new DrawTool (caller owns) */
Tool* draw_tool_create(const char* shape_type /* "LINE", "CIRCLE", "RECT" */);

/* Free a DrawTool */
void draw_tool_destroy(Tool* t);

/* Get current shape type */
const char* draw_tool_get_type(const Tool* t);

/* Set shape type ("LINE", "CIRCLE", "RECT") */
void draw_tool_set_type(Tool* t, const char* shape_type);

#endif /* DRAW_TOOL_H */
