#include <tools/tool_controller.h>

#include <app/workspace.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
#include <document/document.h>

#include <GLFW/glfw3.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ToolDescriptor descriptors[TOOL_MAX_TYPES];
    int count;
    int initialized;
} ToolRegistryState;

typedef struct {
    int dragging;
    int moved;
    Vec2 last_world;
    Vec2 drag_delta_total;
    int drag_object_count;
    ObjectId drag_object_ids[DOCUMENT_MAX_SELECTION];
    unsigned int drag_revision_before;
} SelectToolState;

typedef struct {
    int panning;
} PanToolState;

typedef struct {
    int drawing;
    Vec2 anchor;
    Vec2 current;
} ShapeToolState;

typedef struct {
    const char* object_type_id;
    GraphicObject* (*create_object)(Vec2 anchor, Vec2 current, GraphicStyle style);
    int (*update_object)(GraphicObject* object, Vec2 anchor, Vec2 current);
} ShapeToolConfig;

static ToolRegistryState g_tool_registry = {0};

static void ensure_builtin_tools(void);

#if defined(GLDRAW_ENABLE_SCRIPTING)
void gldraw_register_script_tool(void);
#endif

static void destroy_overlay(Tool* tool)
{
    if (tool && tool->overlay_object) {
        object_destroy(tool->overlay_object);
        tool->overlay_object = NULL;
    }
}

static RectF rect_from_points(Vec2 a, Vec2 b)
{
    RectF rect;
    rect.x = (a.x < b.x) ? a.x : b.x;
    rect.y = (a.y < b.y) ? a.y : b.y;
    rect.w = fabsf(b.x - a.x);
    rect.h = fabsf(b.y - a.y);
    return rect;
}

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
        tool_registry_find(descriptor->id) ||
        g_tool_registry.count >= TOOL_MAX_TYPES) {
        return 0;
    }

    g_tool_registry.descriptors[g_tool_registry.count++] = *descriptor;
    return 1;
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

static int default_tool_create(Tool* tool, size_t state_size)
{
    if (!tool) {
        return 0;
    }
    if (state_size > 0u) {
        tool->state = calloc(1u, state_size);
        if (!tool->state) {
            return 0;
        }
    }
    return 1;
}

static void default_tool_destroy(Tool* tool)
{
    if (!tool) {
        return;
    }
    destroy_overlay(tool);
    free(tool->state);
    tool->state = NULL;
}

static int select_tool_create(Tool* tool, const ToolDescriptor* descriptor)
{
    (void)descriptor;
    return default_tool_create(tool, sizeof(SelectToolState));
}

static int pan_tool_create(Tool* tool, const ToolDescriptor* descriptor)
{
    (void)descriptor;
    return default_tool_create(tool, sizeof(PanToolState));
}

static int shape_tool_create(Tool* tool, const ToolDescriptor* descriptor)
{
    if (!descriptor || !descriptor->user_data) {
        return 0;
    }
    return default_tool_create(tool, sizeof(ShapeToolState));
}

static void select_tool_deactivate(Tool* tool, ToolContext* context)
{
    SelectToolState* state = tool ? (SelectToolState*)tool->state : NULL;
    (void)context;

    if (!state) {
        return;
    }

    state->dragging = 0;
    state->moved = 0;
    state->drag_delta_total = vec2_make(0.0f, 0.0f);
    state->drag_object_count = 0;
    state->drag_revision_before = 0u;
}

static int select_tool_pointer_down(Tool* tool, ToolContext* context, const ToolEvent* event)
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
    state->drag_revision_before = 0u;
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
        if (!selection_set_contains(context->selection, hit->id) || context->selection->count != 1) {
            selection_set_clear(context->selection);
            selection_set_add(context->selection, hit->id);
        }
        state->dragging = selection_set_contains(context->selection, hit->id);
    }

    if (!state->dragging || context->selection->count <= 0) {
        return 0;
    }

    state->drag_object_count = context->selection->count;
    if (state->drag_object_count > DOCUMENT_MAX_SELECTION) {
        state->drag_object_count = DOCUMENT_MAX_SELECTION;
    }
    for (i = 0; i < state->drag_object_count; ++i) {
        state->drag_object_ids[i] = context->selection->ids[i];
    }
    state->last_world = event->world_pos;
    state->drag_revision_before = context->document->revision;
    return 1;
}

static void select_tool_pointer_move(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    SelectToolState* state = tool ? (SelectToolState*)tool->state : NULL;
    Vec2 delta = {0.0f, 0.0f};
    int i = 0;

    if (!state || !context || !event || !state->dragging) {
        return;
    }

    delta = vec2_sub(event->world_pos, state->last_world);
    if (vec2_length_sq(delta) <= 1e-6f) {
        return;
    }

    for (i = 0; i < state->drag_object_count; ++i) {
        GraphicObject* object = document_find_object(context->document, state->drag_object_ids[i]);
        if (object) {
            object_translate(object, delta);
        }
    }

    state->last_world = event->world_pos;
    state->drag_delta_total = vec2_add(state->drag_delta_total, delta);
    state->moved = 1;
    document_touch(context->document);
}

static void select_tool_pointer_up(Tool* tool, ToolContext* context, const ToolEvent* event)
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
            command_executor_record_executed(&context->workspace->core.commands, command);
        }
        workspace_sync_document_dirty(context->workspace);
    }

    state->dragging = 0;
    state->moved = 0;
    state->drag_delta_total = vec2_make(0.0f, 0.0f);
    state->drag_object_count = 0;
    state->drag_revision_before = 0u;
}

static void select_tool_key_down(Tool* tool, ToolContext* context, int key, int mods)
{
    (void)tool;
    (void)mods;
    if (context && key == GLFW_KEY_ESCAPE) {
        selection_set_clear(context->selection);
    }
}

static void pan_tool_deactivate(Tool* tool, ToolContext* context)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    (void)context;
    if (state) {
        state->panning = 0;
    }
}

static int pan_tool_pointer_down(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    (void)context;
    if (!state || !event || event->button != GLFW_MOUSE_BUTTON_LEFT) {
        return 0;
    }
    state->panning = 1;
    return 1;
}

static void pan_tool_pointer_move(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    PanToolState* state = tool ? (PanToolState*)tool->state : NULL;
    if (!state || !context || !event || !state->panning) {
        return;
    }
    canvas_view_pan_screen_delta(context->canvas, event->delta_screen);
}

static void pan_tool_pointer_up(Tool* tool, ToolContext* context, const ToolEvent* event)
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
    if (state && key == GLFW_KEY_ESCAPE) {
        state->panning = 0;
    }
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

static int shape_tool_pointer_down(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    ShapeToolState* state = tool ? (ShapeToolState*)tool->state : NULL;
    const ShapeToolConfig* config = tool && tool->descriptor ? (const ShapeToolConfig*)tool->descriptor->user_data : NULL;
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

static void shape_tool_pointer_move(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    ShapeToolState* state = tool ? (ShapeToolState*)tool->state : NULL;
    const ShapeToolConfig* config = tool && tool->descriptor ? (const ShapeToolConfig*)tool->descriptor->user_data : NULL;

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

static void shape_tool_pointer_up(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    ShapeToolState* state = tool ? (ShapeToolState*)tool->state : NULL;
    const ShapeToolConfig* config = tool && tool->descriptor ? (const ShapeToolConfig*)tool->descriptor->user_data : NULL;
    GraphicStyle style = object_default_style();
    GraphicObject* object = NULL;
    Command* command = NULL;
    ObjectId created_id = 0u;

    (void)event;
    if (!state || !config || !context || !context->workspace || !state->drawing) {
        return;
    }

    state->drawing = 0;
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

static GraphicObject* default_draw_overlay(Tool* tool, ToolContext* context)
{
    (void)context;
    return tool ? tool->overlay_object : NULL;
}

static const ToolDescriptor g_builtin_tools[] = {
    {TOOL_ID_SELECT, "Select", "tool.select", "Select and edit objects", "Select", select_tool_create, default_tool_destroy, NULL, select_tool_deactivate, select_tool_pointer_down, select_tool_pointer_move, select_tool_pointer_up, select_tool_key_down, default_draw_overlay, NULL},
    {TOOL_ID_PAN, "Hand", "tool.pan", "Pan canvas view", "Hand", pan_tool_create, default_tool_destroy, NULL, pan_tool_deactivate, pan_tool_pointer_down, pan_tool_pointer_move, pan_tool_pointer_up, pan_tool_key_down, default_draw_overlay, NULL},
    {TOOL_ID_LINE, "Line", "tool.line", "Draw line", "Line", shape_tool_create, default_tool_destroy, NULL, shape_tool_deactivate, shape_tool_pointer_down, shape_tool_pointer_move, shape_tool_pointer_up, shape_tool_key_down, default_draw_overlay, &g_line_shape_config},
    {TOOL_ID_RECT, "Rectangle", "tool.rect", "Draw rectangle", "Rect", shape_tool_create, default_tool_destroy, NULL, shape_tool_deactivate, shape_tool_pointer_down, shape_tool_pointer_move, shape_tool_pointer_up, shape_tool_key_down, default_draw_overlay, &g_rect_shape_config},
    {TOOL_ID_ELLIPSE, "Ellipse", "tool.ellipse", "Draw ellipse", "Ellipse", shape_tool_create, default_tool_destroy, NULL, shape_tool_deactivate, shape_tool_pointer_down, shape_tool_pointer_move, shape_tool_pointer_up, shape_tool_key_down, default_draw_overlay, &g_ellipse_shape_config}
};

static void ensure_builtin_tools(void)
{
    size_t i = 0;

    tool_registry_init();
    for (i = 0; i < sizeof(g_builtin_tools) / sizeof(g_builtin_tools[0]); ++i) {
        if (!tool_registry_find(g_builtin_tools[i].id)) {
            register_tool(&g_builtin_tools[i]);
        }
    }
#if defined(GLDRAW_ENABLE_SCRIPTING)
    gldraw_register_script_tool();
#endif
}

static int tool_controller_find_index(const ToolController* controller, const char* tool_id)
{
    int i = 0;

    if (!controller || !tool_id) {
        return -1;
    }

    for (i = 0; i < controller->tool_count; ++i) {
        if (controller->tools[i].descriptor &&
            strcmp(controller->tools[i].descriptor->id, tool_id) == 0) {
            return i;
        }
    }

    return -1;
}

void tool_controller_init(ToolController* controller)
{
    int i = 0;

    if (!controller) {
        return;
    }

    memset(controller, 0, sizeof(*controller));
    ensure_builtin_tools();
    controller->tool_count = tool_registry_count();
    controller->tools = (Tool*)calloc((size_t)controller->tool_count, sizeof(controller->tools[0]));
    if (!controller->tools) {
        controller->tool_count = 0;
        controller->active_index = -1;
        return;
    }

    for (i = 0; i < controller->tool_count; ++i) {
        const ToolDescriptor* descriptor = tool_registry_at(i);
        controller->tools[i].descriptor = descriptor;
        if (descriptor && descriptor->create_tool &&
            !descriptor->create_tool(&controller->tools[i], descriptor)) {
            tool_controller_shutdown(controller);
            return;
        }
    }

    controller->active_index = tool_controller_find_index(controller, TOOL_ID_SELECT);
    if (controller->active_index < 0) {
        controller->active_index = (controller->tool_count > 0) ? 0 : -1;
    }
}

void tool_controller_shutdown(ToolController* controller)
{
    int i = 0;

    if (!controller) {
        return;
    }

    for (i = 0; i < controller->tool_count; ++i) {
        Tool* tool = &controller->tools[i];
        if (tool->descriptor && tool->descriptor->destroy_tool) {
            tool->descriptor->destroy_tool(tool);
        } else {
            default_tool_destroy(tool);
        }
    }

    free(controller->tools);
    memset(controller, 0, sizeof(*controller));
    controller->active_index = -1;
}

int tool_controller_set_active(ToolController* controller,
                               ToolContext* context,
                               const char* tool_id)
{
    Tool* current = NULL;
    Tool* next = NULL;
    int next_index = -1;

    if (!controller || !tool_id) {
        return 0;
    }

    next_index = tool_controller_find_index(controller, tool_id);
    if (next_index < 0 || controller->active_index == next_index) {
        return next_index >= 0;
    }

    current = tool_controller_get_active(controller);
    if (current && current->descriptor && current->descriptor->deactivate) {
        current->descriptor->deactivate(current, context);
    }

    controller->active_index = next_index;
    next = tool_controller_get_active(controller);
    if (next && next->descriptor && next->descriptor->activate) {
        next->descriptor->activate(next, context);
    }

    return 1;
}

Tool* tool_controller_get_active(ToolController* controller)
{
    if (!controller || controller->active_index < 0 ||
        controller->active_index >= controller->tool_count) {
        return NULL;
    }
    return &controller->tools[controller->active_index];
}

const char* tool_controller_active_id(const ToolController* controller)
{
    if (!controller || controller->active_index < 0 ||
        controller->active_index >= controller->tool_count ||
        !controller->tools[controller->active_index].descriptor) {
        return NULL;
    }
    return controller->tools[controller->active_index].descriptor->id;
}

const char* tool_controller_active_label(const ToolController* controller)
{
    if (!controller || controller->active_index < 0 ||
        controller->active_index >= controller->tool_count ||
        !controller->tools[controller->active_index].descriptor) {
        return "Unknown";
    }
    return controller->tools[controller->active_index].descriptor->name;
}

GraphicObject* tool_controller_overlay_object(ToolController* controller,
                                              ToolContext* context)
{
    Tool* tool = tool_controller_get_active(controller);
    if (!tool) {
        return NULL;
    }
    if (tool->descriptor && tool->descriptor->draw_overlay) {
        return tool->descriptor->draw_overlay(tool, context);
    }
    return tool->overlay_object;
}

int tool_controller_tool_count(const ToolController* controller)
{
    return controller ? controller->tool_count : 0;
}

const ToolDescriptor* tool_controller_tool_descriptor_at(const ToolController* controller,
                                                         int index)
{
    if (!controller || index < 0 || index >= controller->tool_count) {
        return NULL;
    }
    return controller->tools[index].descriptor;
}

void tool_controller_pointer_down(ToolController* controller,
                                  ToolContext* context,
                                  const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    int accepted = 0;

    if (!controller || !tool || !tool->descriptor || !tool->descriptor->pointer_down) {
        return;
    }

    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
    accepted = tool->descriptor->pointer_down(tool, context, event);
    controller->pointer_captured = accepted ? 1 : 0;
}

void tool_controller_pointer_move(ToolController* controller,
                                  ToolContext* context,
                                  const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    if (!controller || !tool || !tool->descriptor || !tool->descriptor->pointer_move) {
        return;
    }
    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
    tool->descriptor->pointer_move(tool, context, event);
}

void tool_controller_pointer_up(ToolController* controller,
                                ToolContext* context,
                                const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    if (!controller || !tool || !tool->descriptor || !tool->descriptor->pointer_up) {
        return;
    }
    tool->descriptor->pointer_up(tool, context, event);
    controller->pointer_captured = 0;
    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
}

void tool_controller_key_down(ToolController* controller,
                              ToolContext* context,
                              int key,
                              int mods)
{
    Tool* tool = tool_controller_get_active(controller);
    if (tool && tool->descriptor && tool->descriptor->key_down) {
        tool->descriptor->key_down(tool, context, key, mods);
    }
}

void tool_controller_scroll(ToolController* controller,
                            ToolContext* context,
                            Vec2 screen_pos,
                            float yoffset)
{
    float factor = 1.0f;
    (void)controller;

    if (!context || !context->canvas || yoffset == 0.0f) {
        return;
    }

    factor = (yoffset > 0.0f) ? 1.1f : 0.9f;
    canvas_view_zoom_at_screen_point(context->canvas, factor, screen_pos);
}
