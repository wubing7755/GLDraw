#ifndef SELECT_TOOL_H
#define SELECT_TOOL_H

#include <core/tool.h>

/* =============================================================================
 * Phase 3: SelectTool — select and move shapes
 *
 * Left click on shape  → select it
 * Left click on empty → clear selection
 * Drag selected shape → move it
 * Shift+click         → toggle multi-select
 * =============================================================================
 */

typedef struct SelectToolCtx SelectToolCtx;

Tool* select_tool_create(void);
void select_tool_destroy(Tool* t);

#endif /* SELECT_TOOL_H */
