#include "tool_internal.h"

#include <app/workspace_internal.h>
#include <commands/command.h>
#include <document/document.h>

#include <GLFW/glfw3.h>

#include <math.h>
#include <string.h>

typedef struct {
    int drawing;
    Vec2 anchor;
    Vec2 current;
} ShapeToolState;

/* ShapeToolConfig is defined in include/tools/tool.h so extensions can reuse
 * the shape-tool lifecycle without reimplementing pointer_down/move/up. */

static GraphicObject* create_line_shape(Vec2 anchor, Vec2 current, GraphicStyle style)
{
    return object_create_line(anchor, current, style);
}

static GraphicObject* create_rect_shape(Vec2 anchor, Vec2 current, GraphicStyle style)
{
    return object_create_rect(rect_from_points(anchor, current), style);
}

static GraphicObject* create_ellipse_shape(Vec2 anchor, Vec2 current, GraphicStyle style)
{
    return object_create_ellipse(rect_from_points(anchor, current), style);
}

static int update_line_shape(GraphicObject* object, Vec2 anchor, Vec2 current)
{
    return object_type_is(object, "line") &&
           object_set_scalar(object, "x1", anchor.x) &&
           object_set_scalar(object, "y1", anchor.y) &&
           object_set_scalar(object, "x2", current.x) &&
           object_set_scalar(object, "y2", current.y);
}

static int update_rect_shape(GraphicObject* object, Vec2 anchor, Vec2 current)
{
    RectF rect = rect_from_points(anchor, current);
    return object_type_is(object, "rect") &&
           object_set_scalar(object, "x", rect.x) &&
           object_set_scalar(object, "y", rect.y) &&
           object_set_scalar(object, "width", rect.w) &&
           object_set_scalar(object, "height", rect.h);
}

static int update_ellipse_shape(GraphicObject* object, Vec2 anchor, Vec2 current)
{
    RectF rect = rect_from_points(anchor, current);
    return object_type_is(object, "ellipse") &&
           object_set_scalar(object, "x", rect.x) &&
           object_set_scalar(object, "y", rect.y) &&
           object_set_scalar(object, "width", rect.w) &&
           object_set_scalar(object, "height", rect.h);
}

static const ShapeToolConfig g_line_shape_config = {
    "line",
    create_line_shape,
    update_line_shape
};

static const ShapeToolConfig g_rect_shape_config = {
    "rect",
    create_rect_shape,
    update_rect_shape
};

static const ShapeToolConfig g_ellipse_shape_config = {
    "ellipse",
    create_ellipse_shape,
    update_ellipse_shape
};

static int shape_tool_create(Tool* tool, const ToolDescriptor* descriptor)
{
    if (!descriptor || !descriptor->user_data) {
        return 0;
    }
    return default_tool_create(tool, sizeof(ShapeToolState));
}

static void shape_tool_deactivate(Tool* tool, ToolContext* context)
{
    ShapeToolState* state = tool ? (ShapeToolState*)tool->state : NULL;
    (void)context;
    if (!state) {
        return;
    }
    state->drawing = 0;
    destroy_overlay(tool);
}

static int shape_tool_pointer_down(Tool* tool, ToolContext* context,
                                   const ToolEvent* event)
{
    ShapeToolState* state = tool ? (ShapeToolState*)tool->state : NULL;
    const ShapeToolConfig* config = tool && tool->descriptor
        ? (const ShapeToolConfig*)tool->descriptor->user_data : NULL;
    GraphicStyle style = object_default_style();

    if (!state || !config || !event || event->button != GLFW_MOUSE_BUTTON_LEFT) {
        return 0;
    }
    if (!context || !context->document ||
        document_layer_is_locked(context->document,
                                 document_active_layer_id(context->document))) {
        return 0;
    }

    state->drawing = 1;
    state->anchor = event->world_pos;
    state->current = event->world_pos;

    style.stroke_color.a = 0.75f;
    destroy_overlay(tool);
    tool->overlay_object = config->create_object(state->anchor, state->current, style);
    return tool->overlay_object != NULL;
}

static void shape_tool_pointer_move(Tool* tool, ToolContext* context,
                                    const ToolEvent* event)
{
    ShapeToolState* state = tool ? (ShapeToolState*)tool->state : NULL;
    const ShapeToolConfig* config = tool && tool->descriptor
        ? (const ShapeToolConfig*)tool->descriptor->user_data : NULL;

    (void)context;
    if (!state || !config || !event || !state->drawing) {
        return;
    }

    state->current = event->world_pos;
    if (!config->update_object(tool->overlay_object, state->anchor, state->current)) {
        destroy_overlay(tool);
        state->drawing = 0;
    }
}

static void shape_tool_pointer_up(Tool* tool, ToolContext* context,
                                  const ToolEvent* event)
{
    ShapeToolState* state = tool ? (ShapeToolState*)tool->state : NULL;
    const ShapeToolConfig* config = tool && tool->descriptor
        ? (const ShapeToolConfig*)tool->descriptor->user_data : NULL;
    GraphicStyle style = object_default_style();
    GraphicObject* object = NULL;
    Command* command = NULL;
    ObjectId created_id = 0u;

    (void)event;
    if (!state || !config || !context || !context->workspace || !state->drawing) {
        return;
    }

    state->drawing = 0;
    if (state->anchor.x == state->current.x && state->anchor.y == state->current.y) {
        destroy_overlay(tool);
        return;
    }
    object = config->create_object(state->anchor, state->current, style);
    destroy_overlay(tool);
    if (!object) {
        return;
    }

    command = command_create_create_object(object);
    if (!command ||
        !command_executor_execute(&context->workspace->core.commands,
                                  command,
                                  context->document)) {
        return;
    }

    created_id = object->id;
    selection_set_clear(context->selection);
    selection_set_add(context->selection, created_id);
    workspace_sync_document_dirty(context->workspace);
}

static void shape_tool_key_down(Tool* tool, ToolContext* context, int key, int mods)
{
    (void)mods;
    if (key == GLFW_KEY_ESCAPE) {
        shape_tool_deactivate(tool, context);
    }
}

static const ToolDescriptor g_line_tool_descriptor = {
    TOOL_ID_LINE,            /* id */
    "Line",                  /* name */
    "tool.line",             /* command_id */
    "Draw line",             /* tooltip */
    "Line",                  /* icon */
    "L",                     /* default_shortcut */
    1,                       /* requires_editable_layer */
    shape_tool_create,
    default_tool_destroy,
    NULL,                    /* activate */
    shape_tool_deactivate,
    shape_tool_pointer_down,
    shape_tool_pointer_move,
    shape_tool_pointer_up,
    shape_tool_key_down,
    default_draw_overlay,
    &g_line_shape_config
};

static const ToolDescriptor g_rect_tool_descriptor = {
    TOOL_ID_RECT,            /* id */
    "Rectangle",             /* name */
    "tool.rect",             /* command_id */
    "Draw rectangle",        /* tooltip */
    "Rect",                  /* icon */
    "R",                     /* default_shortcut */
    1,                       /* requires_editable_layer */
    shape_tool_create,
    default_tool_destroy,
    NULL,                    /* activate */
    shape_tool_deactivate,
    shape_tool_pointer_down,
    shape_tool_pointer_move,
    shape_tool_pointer_up,
    shape_tool_key_down,
    default_draw_overlay,
    &g_rect_shape_config
};

static const ToolDescriptor g_ellipse_tool_descriptor = {
    TOOL_ID_ELLIPSE,         /* id */
    "Ellipse",               /* name */
    "tool.ellipse",          /* command_id */
    "Draw ellipse",          /* tooltip */
    "Ellipse",               /* icon */
    "E",                     /* default_shortcut */
    1,                       /* requires_editable_layer */
    shape_tool_create,
    default_tool_destroy,
    NULL,                    /* activate */
    shape_tool_deactivate,
    shape_tool_pointer_down,
    shape_tool_pointer_move,
    shape_tool_pointer_up,
    shape_tool_key_down,
    default_draw_overlay,
    &g_ellipse_shape_config
};

int register_shape_tool_line(void)
{
    /* register_tool() already checks for duplicate IDs */
    return register_tool(&g_line_tool_descriptor);
}

int register_shape_tool_rect(void)
{
    return register_tool(&g_rect_tool_descriptor);
}

int register_shape_tool_ellipse(void)
{
    return register_tool(&g_ellipse_tool_descriptor);
}

int register_shape_tool(const char* tool_id,
                        const char* name,
                        const char* shortcut,
                        int requires_editable,
                        const ShapeToolConfig* config)
{
    ToolDescriptor desc;

    if (!tool_id || !name || !config) {
        return 0;
    }
    /* register_tool() already checks for duplicate IDs */
    memset(&desc, 0, sizeof(desc));
    desc.id = tool_id;
    desc.name = name;
    desc.command_id = name;
    desc.tooltip = name;
    desc.icon = name;
    desc.default_shortcut = shortcut;
    desc.requires_editable_layer = requires_editable;
    desc.create_tool = shape_tool_create;
    desc.destroy_tool = default_tool_destroy;
    desc.deactivate = shape_tool_deactivate;
    desc.pointer_down = shape_tool_pointer_down;
    desc.pointer_move = shape_tool_pointer_move;
    desc.pointer_up = shape_tool_pointer_up;
    desc.key_down = shape_tool_key_down;
    desc.draw_overlay = default_draw_overlay;
    desc.user_data = config;

    return register_tool(&desc);
}
