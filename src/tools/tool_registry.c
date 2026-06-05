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
    int builtins_registered;
    int builtins_registering;
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

static int tool_registry_string_equal(const char* a, const char* b)
{
    if (a == b) {
        return 1;
    }
    if (!a || !b) {
        return 0;
    }
    return strcmp(a, b) == 0;
}

static int tool_registry_descriptor_matches(const ToolDescriptor* a,
                                            const ToolDescriptor* b)
{
    return a && b &&
           tool_registry_string_equal(a->id, b->id) &&
           tool_registry_string_equal(a->name, b->name) &&
           tool_registry_string_equal(a->command_id, b->command_id) &&
           tool_registry_string_equal(a->tooltip, b->tooltip) &&
           tool_registry_string_equal(a->icon, b->icon) &&
           tool_registry_string_equal(a->default_shortcut, b->default_shortcut) &&
           a->requires_editable_layer == b->requires_editable_layer &&
           a->create_tool == b->create_tool &&
           a->destroy_tool == b->destroy_tool &&
           a->activate == b->activate &&
           a->deactivate == b->deactivate &&
           a->pointer_down == b->pointer_down &&
           a->pointer_move == b->pointer_move &&
           a->pointer_up == b->pointer_up &&
           a->key_down == b->key_down &&
           a->draw_overlay == b->draw_overlay &&
           a->user_data == b->user_data;
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
    const ToolDescriptor* existing = NULL;

    tool_registry_init();

    if (!descriptor || !descriptor->id || descriptor->id[0] == '\0' ||
        !descriptor->name || descriptor->name[0] == '\0') {
        return 0;
    }

    existing = tool_registry_find(descriptor->id);
    if (existing) {
        return tool_registry_descriptor_matches(existing, descriptor);
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
    int ok = 0;

    tool_registry_init();
    if (g_tool_registry.builtins_registered) {
        return 1;
    }
    if (g_tool_registry.builtins_registering) {
        return 1;
    }

    g_tool_registry.builtins_registering = 1;
    ok = tool_manifest_register_all();
    g_tool_registry.builtins_registering = 0;
    if (ok) {
        g_tool_registry.builtins_registered = 1;
    }
    return ok;
}

static void ensure_builtin_tools(void)
{
    tool_registry_init();
    if (!g_tool_registry.builtins_registered &&
        !g_tool_registry.builtins_registering) {
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
