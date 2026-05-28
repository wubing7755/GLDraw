/**
 * @file command_catalog.h
 * @brief Stable editor command metadata and lookup helpers.
 */
#ifndef GLDRAW_APP_COMMAND_CATALOG_H
#define GLDRAW_APP_COMMAND_CATALOG_H

#include <app/command_types.h>
#include <tools/tool.h>

/** Convert a registered tool index into its dynamic editor command value. */
EditorCommand command_catalog_tool_command(int tool_index);

/** Number of stable, non-tool commands in the built-in catalog. */
int command_catalog_stable_count(void);

/** Iterate stable, non-tool command metadata. Returns NULL for invalid index. */
const CommandDescriptor* command_catalog_stable_at(int index);

/**
 * Look up the registered tool descriptor for a dynamic tool command.
 * `out_tool_index` may be NULL.
 */
const ToolDescriptor* command_catalog_tool_for_command(EditorCommand command,
                                                       int* out_tool_index);

/** Look up command metadata by stable command identifier. */
const CommandDescriptor* command_catalog_find_by_id(const char* command_id);

/** Look up command metadata by command enum/runtime value. */
const CommandDescriptor* command_catalog_find_by_command(EditorCommand command);

/** Look up command metadata by UI menu action ID. */
const CommandDescriptor* command_catalog_find_by_menu_id(int id);

#endif /* GLDRAW_APP_COMMAND_CATALOG_H */
