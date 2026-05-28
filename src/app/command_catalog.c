/**
 * @file command_catalog.c
 * @brief Stable command metadata and dynamic tool command descriptors.
 */
#include <app/command_catalog.h>

#include "command_definitions.h"

#include <ui/ui_menu_def.h>

#include <string.h>

#define TOOL_DESCRIPTOR_CACHE_SIZE 4

static const CommandDescriptor g_commands[] = {
#define GLDRAW_COMMAND_DESCRIPTOR(command, id, label, scope, menu_id, tool_id, availability, service, execute) \
    {command, id, label, scope, menu_id, tool_id},
    GLDRAW_STABLE_COMMANDS(GLDRAW_COMMAND_DESCRIPTOR)
#undef GLDRAW_COMMAND_DESCRIPTOR
};

EditorCommand command_catalog_tool_command(int tool_index)
{
    return EDITOR_COMMAND_DYNAMIC_TOOL_BASE + tool_index;
}

int command_catalog_stable_count(void)
{
    return (int)(sizeof(g_commands) / sizeof(g_commands[0]));
}

const CommandDescriptor* command_catalog_stable_at(int index)
{
    if (index < 0 || index >= command_catalog_stable_count()) {
        return NULL;
    }

    return &g_commands[index];
}

static const CommandDescriptor* command_catalog_make_tool_descriptor(const ToolDescriptor* tool,
                                                                    int tool_index)
{
    static CommandDescriptor cache[TOOL_DESCRIPTOR_CACHE_SIZE];
    static int cache_index = 0;
    CommandDescriptor* descriptor;

    if (!tool) {
        return NULL;
    }

    descriptor = &cache[cache_index];
    cache_index = (cache_index + 1) % TOOL_DESCRIPTOR_CACHE_SIZE;

    descriptor->command = command_catalog_tool_command(tool_index);
    descriptor->id = tool->command_id;
    descriptor->label = tool->name;
    descriptor->scope = KEY_SCOPE_GLOBAL;
    descriptor->menu_id = MENU_ID_TOOL_DYNAMIC_BASE + tool_index;
    descriptor->tool_id = tool->id;
    return descriptor;
}

const ToolDescriptor* command_catalog_tool_for_command(EditorCommand command,
                                                       int* out_tool_index)
{
    int tool_index = command - EDITOR_COMMAND_DYNAMIC_TOOL_BASE;

    if (out_tool_index) {
        *out_tool_index = -1;
    }
    if (tool_index < 0 || tool_index >= tool_registry_count()) {
        return NULL;
    }
    if (out_tool_index) {
        *out_tool_index = tool_index;
    }
    return tool_registry_at(tool_index);
}

static const CommandDescriptor* command_catalog_find_dynamic_tool_by_id(const char* command_id)
{
    int i = 0;

    if (!command_id) {
        return NULL;
    }

    for (i = 0; i < tool_registry_count(); ++i) {
        const ToolDescriptor* tool = tool_registry_at(i);
        if (tool && tool->command_id && strcmp(tool->command_id, command_id) == 0) {
            return command_catalog_make_tool_descriptor(tool, i);
        }
    }

    return NULL;
}

const CommandDescriptor* command_catalog_find_by_id(const char* command_id)
{
    size_t i = 0u;

    if (!command_id) {
        return NULL;
    }

    for (i = 0u; i < (size_t)command_catalog_stable_count(); ++i) {
        if (strcmp(g_commands[i].id, command_id) == 0) {
            return &g_commands[i];
        }
    }

    return command_catalog_find_dynamic_tool_by_id(command_id);
}

const CommandDescriptor* command_catalog_find_by_command(EditorCommand command)
{
    size_t i = 0u;

    for (i = 0u; i < (size_t)command_catalog_stable_count(); ++i) {
        if (g_commands[i].command == command) {
            return &g_commands[i];
        }
    }

    return command_catalog_make_tool_descriptor(
        command_catalog_tool_for_command(command, NULL),
        command - EDITOR_COMMAND_DYNAMIC_TOOL_BASE);
}

const CommandDescriptor* command_catalog_find_by_menu_id(int id)
{
    size_t i = 0u;

    if (id < 0) {
        return NULL;
    }

    for (i = 0u; i < (size_t)command_catalog_stable_count(); ++i) {
        if (g_commands[i].menu_id == id) {
            return &g_commands[i];
        }
    }

    if (id >= MENU_ID_TOOL_DYNAMIC_BASE) {
        return command_catalog_make_tool_descriptor(
            tool_registry_at(id - MENU_ID_TOOL_DYNAMIC_BASE),
            id - MENU_ID_TOOL_DYNAMIC_BASE);
    }

    return NULL;
}
