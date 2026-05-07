#include <tools/tool_controller.h>

#include <base/math2d.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

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

int tool_controller_is_pointer_captured(const ToolController* controller)
{
    return controller ? controller->pointer_captured : 0;
}

Vec2 tool_controller_last_screen(const ToolController* controller)
{
    return controller ? controller->last_screen : vec2_make(0.0f, 0.0f);
}

Vec2 tool_controller_last_world(const ToolController* controller)
{
    return controller ? controller->last_world : vec2_make(0.0f, 0.0f);
}

void tool_controller_set_pointer_anchor(ToolController* controller,
                                        Vec2 screen_pos,
                                        Vec2 world_pos)
{
    if (!controller) {
        return;
    }

    controller->last_screen = screen_pos;
    controller->last_world = world_pos;
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
