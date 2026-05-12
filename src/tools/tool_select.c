#include "tool_internal.h"

#include <app/workspace_internal.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>

#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <string.h>

typedef struct {
    int dragging;
    int moved;
    Vec2 last_world;
    Vec2 drag_delta_total;
    int drag_object_count;
    int drag_object_capacity;
    ObjectId* drag_object_ids;
} SelectToolState;

static int select_tool_create(Tool* tool, const ToolDescriptor* descriptor)
{
    (void)descriptor;
    return default_tool_create(tool, sizeof(SelectToolState));
}

static void select_tool_destroy(Tool* tool)
{
    SelectToolState* state = tool ? (SelectToolState*)tool->state : NULL;
    if (state) {
        free(state->drag_object_ids);
        state->drag_object_ids = NULL;
        state->drag_object_capacity = 0;
    }
    default_tool_destroy(tool);
}

static void select_tool_deactivate(Tool* tool, ToolContext* context)
{
    SelectToolState* state = tool ? (SelectToolState*)tool->state : NULL;

    if (!state) {
        return;
    }

    if (context && context->workspace) {
        context->workspace->session.selection_preview_active = 0;
        context->workspace->session.selection_preview_delta = vec2_make(0.0f, 0.0f);
    }
    state->dragging = 0;
    state->moved = 0;
    state->drag_delta_total = vec2_make(0.0f, 0.0f);
    state->drag_object_count = 0;
}

static int select_tool_reserve_drag_ids(SelectToolState* state, int needed)
{
    ObjectId* ids = NULL;
    int capacity = 0;

    if (!state || needed <= 0) {
        return state != NULL;
    }
    if (needed <= state->drag_object_capacity) {
        return 1;
    }

    capacity = state->drag_object_capacity > 0 ? state->drag_object_capacity : 8;
    while (capacity < needed) {
        capacity *= 2;
    }

    ids = (ObjectId*)realloc(state->drag_object_ids,
                             (size_t)capacity * sizeof(state->drag_object_ids[0]));
    if (!ids) {
        return 0;
    }

    state->drag_object_ids = ids;
    state->drag_object_capacity = capacity;
    return 1;
}

static int select_tool_pointer_down(Tool* tool, ToolContext* context,
                                    const ToolEvent* event)
{
    SelectToolState* state = tool ? (SelectToolState*)tool->state : NULL;
    GraphicObject* hit = NULL;
    int i = 0;

    if (!state || !context || !event || event->button != GLFW_MOUSE_BUTTON_LEFT) {
        return 0;
    }

    state->dragging = 0;
    state->moved = 0;
    state->drag_delta_total = vec2_make(0.0f, 0.0f);
    state->drag_object_count = 0;
    if (context->workspace) {
        context->workspace->session.selection_preview_active = 0;
        context->workspace->session.selection_preview_delta = vec2_make(0.0f, 0.0f);
    }
    hit = canvas_view_pick_object(context->canvas, event->screen_pos, 8.0f);
    if (!hit) {
        if ((event->mods & GLFW_MOD_SHIFT) == 0) {
            selection_set_clear(context->selection);
        }
        return 0;
    }

    if ((event->mods & GLFW_MOD_SHIFT) != 0) {
        selection_set_toggle(context->selection, hit->id);
        state->dragging = selection_set_contains(context->selection, hit->id);
    } else {
        if (!selection_set_contains(context->selection, hit->id) ||
            context->selection->count != 1) {
            selection_set_clear(context->selection);
            selection_set_add(context->selection, hit->id);
        }
        state->dragging = selection_set_contains(context->selection, hit->id);
    }

    if (!state->dragging || context->selection->count <= 0) {
        return 0;
    }

    state->drag_object_count = context->selection->count;
    if (!select_tool_reserve_drag_ids(state, state->drag_object_count)) {
        state->dragging = 0;
        state->drag_object_count = 0;
        return 0;
    }
    for (i = 0; i < state->drag_object_count; ++i) {
        state->drag_object_ids[i] = context->selection->ids[i];
    }
    state->last_world = event->world_pos;
    return 1;
}

static void select_tool_pointer_move(Tool* tool, ToolContext* context,
                                     const ToolEvent* event)
{
    SelectToolState* state = tool ? (SelectToolState*)tool->state : NULL;
    Vec2 delta = {0.0f, 0.0f};
    if (!state || !context || !event || !state->dragging) {
        return;
    }

    delta = vec2_sub(event->world_pos, state->last_world);
    if (vec2_length_sq(delta) <= 1e-6f) {
        return;
    }

    state->last_world = event->world_pos;
    state->drag_delta_total = vec2_add(state->drag_delta_total, delta);
    state->moved = 1;
    if (context->workspace) {
        context->workspace->session.selection_preview_active = 1;
        context->workspace->session.selection_preview_delta = state->drag_delta_total;
    }
}

static void select_tool_pointer_up(Tool* tool, ToolContext* context,
                                   const ToolEvent* event)
{
    SelectToolState* state = tool ? (SelectToolState*)tool->state : NULL;
    (void)event;

    if (!state) {
        return;
    }

    if (state->dragging &&
        state->moved &&
        state->drag_object_count > 0 &&
        vec2_length_sq(state->drag_delta_total) > 1e-6f &&
        context &&
        context->workspace) {
        Command* command = command_create_move_objects(state->drag_object_ids,
                                                       state->drag_object_count,
                                                       state->drag_delta_total);
        if (command) {
            command_executor_execute(&context->workspace->core.commands,
                                     command,
                                     context->document);
        }
        workspace_sync_document_dirty(context->workspace);
    }

    if (context && context->workspace) {
        context->workspace->session.selection_preview_active = 0;
        context->workspace->session.selection_preview_delta = vec2_make(0.0f, 0.0f);
    }
    state->dragging = 0;
    state->moved = 0;
    state->drag_delta_total = vec2_make(0.0f, 0.0f);
    state->drag_object_count = 0;
}

static void select_tool_key_down(Tool* tool, ToolContext* context, int key, int mods)
{
    (void)tool;
    (void)mods;
    if (context && key == GLFW_KEY_ESCAPE) {
        selection_set_clear(context->selection);
    }
}

static const ToolDescriptor g_select_tool_descriptor = {
    TOOL_ID_SELECT,          /* id */
    "Select",                /* name */
    "tool.select",           /* command_id */
    "Select and edit objects", /* tooltip */
    "Select",                /* icon */
    "V",                     /* default_shortcut */
    0,                       /* requires_editable_layer */
    select_tool_create,
    select_tool_destroy,
    NULL,                    /* activate */
    select_tool_deactivate,
    select_tool_pointer_down,
    select_tool_pointer_move,
    select_tool_pointer_up,
    select_tool_key_down,
    default_draw_overlay,
    NULL                     /* user_data */
};

int register_select_tool(void)
{
    /* register_tool() already checks for duplicate IDs */
    return register_tool(&g_select_tool_descriptor);
}
