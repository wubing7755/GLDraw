#include <tools/tool_controller.h>

#include <base/math2d.h>
#include <canvas/canvas_view.h>

#include <GLFW/glfw3.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TOOL_REGISTRY_INITIAL_CAPACITY 8

typedef struct {
    ToolDescriptor *descriptors;
    int count;
    int capacity;
    int initialized;
} ToolRegistryState;

static ToolRegistryState g_tool_registry = {0};

#if defined(GLDRAW_ENABLE_SCRIPTING)
void gldraw_register_script_tool(void);
#endif

/* ---------------------------------------------------------------------------
 * Shared helpers (also used by individual tool files via tool_internal.h)
 * --------------------------------------------------------------------------- */

void destroy_overlay(Tool* tool)
{
    if (tool && tool->overlay_object) {
        object_destroy(tool->overlay_object);
        tool->overlay_object = NULL;
    }
}

RectF rect_from_points(Vec2 a, Vec2 b)
{
    RectF rect;
    rect.x = (a.x < b.x) ? a.x : b.x;
    rect.y = (a.y < b.y) ? a.y : b.y;
    rect.w = fabsf(b.x - a.x);
    rect.h = fabsf(b.y - a.y);
    return rect;
}

int default_tool_create(Tool* tool, size_t state_size)
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

void default_tool_destroy(Tool* tool)
{
    if (!tool) {
        return;
    }
    destroy_overlay(tool);
    free(tool->state);
    tool->state = NULL;
}

GraphicObject* default_draw_overlay(Tool* tool, ToolContext* context)
{
    (void)context;
    return tool ? tool->overlay_object : NULL;
}

/* ---------------------------------------------------------------------------
 * Tool registry (global singleton)
 * --------------------------------------------------------------------------- */

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
        ToolDescriptor *new_descriptors =
            (ToolDescriptor *)realloc(g_tool_registry.descriptors,
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

/* ---------------------------------------------------------------------------
 * Built-in tool registration — delegates to individual tool files
 * --------------------------------------------------------------------------- */

/* Forward declarations - tool registration functions from individual tool files. */
int register_select_tool(void);
int register_pan_tool(void);
int register_shape_tool_line(void);
int register_shape_tool_rect(void);
int register_shape_tool_ellipse(void);

int register_builtin_tools(void)
{
    /* Manifest-style list: each entry is a single register_*() call.
     * Adding a new built-in tool requires only adding its register_*()
     * function to this array. */
    typedef int (*ToolInitFn)(void);
    static const ToolInitFn manifest[] = {
        register_select_tool,
        register_pan_tool,
        register_shape_tool_line,
        register_shape_tool_rect,
        register_shape_tool_ellipse,
    };
    size_t i = 0;

    tool_registry_init();
    for (i = 0; i < sizeof(manifest) / sizeof(manifest[0]); ++i) {
        if (!manifest[i]()) {
            return 0;
        }
    }

#if defined(GLDRAW_ENABLE_SCRIPTING)
    gldraw_register_script_tool();
#endif
    return 1;
}

/* ---------------------------------------------------------------------------
 * ToolController — active tool management and input dispatch
 * --------------------------------------------------------------------------- */

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
