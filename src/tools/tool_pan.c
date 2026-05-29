#include "tool_internal.h"

#include <canvas/canvas_view.h>

typedef struct {
    int panning;
} PanToolState;

static int pan_tool_create(Tool* tool, const ToolDescriptor* descriptor)
{
    (void)descriptor;
    return default_tool_create(tool, sizeof(PanToolState));
}

static void pan_tool_deactivate(Tool* tool, ToolContext* context)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    (void)context;
    if (state) {
        state->panning = 0;
    }
}

static int pan_tool_pointer_down(Tool* tool, ToolContext* context,
                                 const ToolEvent* event)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    (void)context;
    if (!state || !event || event->button != TOOL_POINTER_BUTTON_LEFT) {
        return 0;
    }
    state->panning = 1;
    return 1;
}

static void pan_tool_pointer_move(Tool* tool, ToolContext* context,
                                  const ToolEvent* event)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    if (!state || !context || !event || !state->panning) {
        return;
    }
    canvas_view_pan_screen_delta(context->canvas, event->delta_screen);
}

static void pan_tool_pointer_up(Tool* tool, ToolContext* context,
                                const ToolEvent* event)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    (void)context;
    (void)event;
    if (state) {
        state->panning = 0;
    }
}

static void pan_tool_key_down(Tool* tool, ToolContext* context, int key, int mods)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    (void)context;
    (void)mods;
    if (state && key == TOOL_INPUT_KEY_ESCAPE) {
        state->panning = 0;
    }
}

static const ToolDescriptor g_pan_tool_descriptor = {
    TOOL_ID_PAN,             /* id */
    "Hand",                  /* name */
    "tool.pan",              /* command_id */
    "Pan canvas view",       /* tooltip */
    "Hand",                  /* icon */
    "H",                     /* default_shortcut */
    0,                       /* requires_editable_layer */
    pan_tool_create,
    default_tool_destroy,
    NULL,                    /* activate */
    pan_tool_deactivate,
    pan_tool_pointer_down,
    pan_tool_pointer_move,
    pan_tool_pointer_up,
    pan_tool_key_down,
    default_draw_overlay,
    NULL                     /* user_data */
};

int register_pan_tool(void)
{
    /* register_tool() already checks for duplicate IDs */
    return register_tool(&g_pan_tool_descriptor);
}
