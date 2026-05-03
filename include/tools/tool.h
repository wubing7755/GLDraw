/**
 * @file tool.h
 * @brief Tool descriptor, registry, and event interfaces.
 */
#ifndef GLDRAW_TOOLS_TOOL_H
#define GLDRAW_TOOLS_TOOL_H

#include <base/types.h>
#include <document/document.h>
#include <document/object.h>

struct Workspace;
struct Document;
struct CanvasView;

#define TOOL_ID_SELECT "select"
#define TOOL_ID_PAN "pan"
#define TOOL_ID_LINE "line"
#define TOOL_ID_RECT "rect"
#define TOOL_ID_ELLIPSE "ellipse"

/* ShapeToolConfig: reusable configuration for shape-drawing tools.
 * Extension tools can provide their own config and call register_shape_tool()
 * to get a fully-functional drawing tool without reimplementing the
 * pointer_down/move/up lifecycle. */
typedef struct {
    const char* object_type_id;
    GraphicObject* (*create_object)(Vec2 anchor, Vec2 current, GraphicStyle style);
    int (*update_object)(GraphicObject* object, Vec2 anchor, Vec2 current);
} ShapeToolConfig;

int register_shape_tool(const char* tool_id,
                        const char* name,
                        const char* shortcut,
                        int requires_editable,
                        const ShapeToolConfig* config);

typedef struct {
    Vec2 screen_pos;
    Vec2 world_pos;
    Vec2 delta_screen;
    Vec2 delta_world;
    int button;
    int mods;
    float wheel_y;
} ToolEvent;

typedef struct {
    struct Workspace* workspace;
    struct Document* document;
    struct CanvasView* canvas;
    SelectionSet* selection;
} ToolContext;

typedef struct Tool Tool;
typedef struct ToolDescriptor ToolDescriptor;

typedef int (*ToolCreateFn)(Tool* tool, const ToolDescriptor* descriptor);
typedef void (*ToolDestroyFn)(Tool* tool);
typedef void (*ToolActivateFn)(Tool* tool, ToolContext* context);
typedef void (*ToolDeactivateFn)(Tool* tool, ToolContext* context);
typedef int (*ToolPointerDownFn)(Tool* tool, ToolContext* context, const ToolEvent* event);
typedef void (*ToolPointerMoveFn)(Tool* tool, ToolContext* context, const ToolEvent* event);
typedef void (*ToolPointerUpFn)(Tool* tool, ToolContext* context, const ToolEvent* event);
typedef void (*ToolKeyDownFn)(Tool* tool, ToolContext* context, int key, int mods);
typedef GraphicObject* (*ToolDrawOverlayFn)(Tool* tool, ToolContext* context);

struct ToolDescriptor {
    const char* id;
    const char* name;
    const char* command_id;
    const char* tooltip;
    const char* icon;
    const char* default_shortcut;
    int requires_editable_layer;
    ToolCreateFn create_tool;
    ToolDestroyFn destroy_tool;
    ToolActivateFn activate;
    ToolDeactivateFn deactivate;
    ToolPointerDownFn pointer_down;
    ToolPointerMoveFn pointer_move;
    ToolPointerUpFn pointer_up;
    ToolKeyDownFn key_down;
    ToolDrawOverlayFn draw_overlay;
    const void* user_data;
};

struct Tool {
    const ToolDescriptor* descriptor;
    void* state;
    GraphicObject* overlay_object;
};

void tool_registry_init(void);
int register_tool(const ToolDescriptor* descriptor);
const ToolDescriptor* tool_registry_lookup(const char* tool_id);
int tool_registry_count(void);
const ToolDescriptor* tool_registry_at(int index);

#endif /* GLDRAW_TOOLS_TOOL_H */
