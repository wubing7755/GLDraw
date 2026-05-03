#include <app/workspace.h>
#include <tools/tool.h>

#include <script/script_runtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ScriptRuntime runtime;
    char script_path[260];
} ScriptToolState;

static int script_tool_create(Tool* tool, const ToolDescriptor* descriptor)
{
    ScriptToolState* state = NULL;
    const char* script_path = descriptor && descriptor->user_data
                                  ? (const char*)descriptor->user_data
                                  : "scripts/star_wreath.lua";

    if (!tool) {
        return 0;
    }

    state = (ScriptToolState*)calloc(1u, sizeof(*state));
    if (!state) {
        return 0;
    }
    if (!script_runtime_init(&state->runtime)) {
        free(state);
        return 0;
    }

    snprintf(state->script_path, sizeof(state->script_path), "%s", script_path);
    tool->state = state;
    return 1;
}

static void script_tool_destroy(Tool* tool)
{
    ScriptToolState* state = tool ? (ScriptToolState*)tool->state : NULL;

    if (!state) {
        return;
    }

    script_runtime_shutdown(&state->runtime);
    free(state);
    tool->state = NULL;
}

static void script_tool_dispatch(Tool* tool,
                                 ToolContext* context,
                                 const ToolEvent* event,
                                 const char* event_name)
{
    ScriptToolState* state = tool ? (ScriptToolState*)tool->state : NULL;

    if (!state || !context || !event || !context->workspace) {
        return;
    }

    script_runtime_set_context(&state->runtime,
                               context->document,
                               context->selection,
                               &context->workspace->core.commands);
    script_runtime_execute_file_event(&state->runtime,
                                      state->script_path,
                                      event_name,
                                      event);
}

static int script_tool_pointer_down(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    script_tool_dispatch(tool, context, event, "pointer_down");
    return 0;
}

static void script_tool_pointer_move(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    script_tool_dispatch(tool, context, event, "pointer_move");
}

static void script_tool_pointer_up(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    script_tool_dispatch(tool, context, event, "pointer_up");
}

static const ToolDescriptor G_SCRIPT_TOOL_DESCRIPTOR = {
    "script",
    "Script",
    NULL,
    "Execute Lua tool script",
    "Script",
    NULL,
    0,
    script_tool_create,
    script_tool_destroy,
    NULL,
    NULL,
    script_tool_pointer_down,
    script_tool_pointer_move,
    script_tool_pointer_up,
    NULL,
    NULL,
    "scripts/star_wreath.lua"
};

void gldraw_register_script_tool(void)
{
    if (!tool_registry_lookup(G_SCRIPT_TOOL_DESCRIPTOR.id)) {
        register_tool(&G_SCRIPT_TOOL_DESCRIPTOR);
    }
}
