#ifndef TOOL_MANAGER_H
#define TOOL_MANAGER_H

#include <core/tool.h>

/* =============================================================================
 * Phase 3: ToolManager — manages current tool and tool switching
 * =============================================================================
 */

void toolmanager_init(void);
void toolmanager_shutdown(void);

/* Get the currently active tool */
Tool* toolmanager_get_current(void);

/* Switch to a different tool */
void toolmanager_set_tool(Tool* new_tool);

#endif /* TOOL_MANAGER_H */
