/**
 * @file tool_registry.c
 * @brief Tool descriptor registry storage plus compatibility registration entrypoint.
 *
 * This module owns descriptor storage/query only. Built-in tool assembly lives
 * in the app-layer manifest (`app/tool_manifest.c`), and
 * `register_builtin_tools()` remains as a compatibility wrapper.
 */
#include <app/tool_manifest.h>
#include <tools/tool_controller.h>

#include <stdlib.h>
#include <string.h>

#define TOOL_REGISTRY_INITIAL_CAPACITY 8

typedef struct {
    ToolDescriptor* descriptors;
    int count;
    int capacity;
    int initialized;
} ToolRegistryState;

static ToolRegistryState g_tool_registry = {0};

static const ToolDescriptor* tool_registry_find(const char* tool_id)
{
    int i = 0;

    if (!tool_id || tool_id[0] == '\0') {
        return NULL;
    }

    for (i = 0; i < g_tool_registry.count; ++i) {
        if (g_tool_registry.descriptors[i].id &&
            strcmp(g_tool_registry.descriptors[i].id, tool_id) == 0) {
            return &g_tool_registry.descriptors[i];
        }
    }

    return NULL;
}

void tool_registry_init(void)
{
    if (!g_tool_registry.initialized) {
        memset(&g_tool_registry, 0, sizeof(g_tool_registry));
        g_tool_registry.initialized = 1;
    }
}

int register_tool(const ToolDescriptor* descriptor)
{
    if (!descriptor || !descriptor->id || descriptor->id[0] == '\0' ||
        !descriptor->name || descriptor->name[0] == '\0' ||
        tool_registry_find(descriptor->id)) {
        return 0;
    }

    if (g_tool_registry.count >= g_tool_registry.capacity) {
        int new_capacity = g_tool_registry.capacity > 0 ? g_tool_registry.capacity * 2
                                                        : TOOL_REGISTRY_INITIAL_CAPACITY;
        ToolDescriptor* new_descriptors =
            (ToolDescriptor*)realloc(g_tool_registry.descriptors,
                                     (size_t)new_capacity *
                                         sizeof(g_tool_registry.descriptors[0]));
        if (!new_descriptors) {
            return 0;
        }
        g_tool_registry.descriptors = new_descriptors;
        g_tool_registry.capacity = new_capacity;
    }

    g_tool_registry.descriptors[g_tool_registry.count++] = *descriptor;
    return 1;
}

int register_builtin_tools(void)
{
    return tool_manifest_register_all();
}

static void ensure_builtin_tools(void)
{
    tool_registry_init();
    if (g_tool_registry.count == 0) {
        register_builtin_tools();
    }
}

const ToolDescriptor* tool_registry_lookup(const char* tool_id)
{
    ensure_builtin_tools();
    return tool_registry_find(tool_id);
}

int tool_registry_count(void)
{
    ensure_builtin_tools();
    return g_tool_registry.count;
}

const ToolDescriptor* tool_registry_at(int index)
{
    ensure_builtin_tools();
    if (index < 0 || index >= g_tool_registry.count) {
        return NULL;
    }
    return &g_tool_registry.descriptors[index];
}
