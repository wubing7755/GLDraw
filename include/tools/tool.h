/**
 * @file tool.h
 * @brief Generic tool abstraction and event payload types.
 *
 * Role in project:
 * - Defines polymorphic tool vtable used by the tool controller.
 * - Carries per-event input state in screen/world coordinates.
 *
 * Module relationships:
 * - Implemented by `tool_controller.c` built-in tools.
 * - Consumes workspace/document/canvas/history through `ToolContext`.
 */
#ifndef GLDRAW_TOOLS_TOOL_H
#define GLDRAW_TOOLS_TOOL_H

#include <base/types.h>
#include <document/object.h>

struct Workspace;
struct Document;
struct CanvasView;
struct DocumentHistory;

/** Built-in tool identifiers. */
typedef enum {
    TOOL_KIND_SELECT = 0,
    TOOL_KIND_PAN,
    TOOL_KIND_LINE,
    TOOL_KIND_RECT,
    TOOL_KIND_ELLIPSE,
    TOOL_KIND_COUNT
} ToolKind;

/** Normalized pointer/keyboard event payload passed to tools. */
typedef struct {
    Vec2 screen_pos;
    Vec2 world_pos;
    Vec2 delta_screen;
    Vec2 delta_world;
    int button;
    int mods;
    float wheel_y;
} ToolEvent;

/** Shared handles a tool needs to mutate editor state safely. */
typedef struct {
    struct Workspace* workspace;
    struct Document* document;
    struct DocumentHistory* history;
    struct CanvasView* canvas;
} ToolContext;

typedef struct Tool Tool;
typedef struct ToolVTable ToolVTable;

struct ToolVTable {
    const char* (*label)(const Tool* tool);
    void (*activate)(Tool* tool, ToolContext* context);
    void (*deactivate)(Tool* tool, ToolContext* context);
    void (*pointer_down)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*pointer_move)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*pointer_up)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*key_down)(Tool* tool, ToolContext* context, int key, int mods);
};

struct Tool {
    ToolKind kind;
    const ToolVTable* vtable;
    void* state;
    GraphicObject* overlay_object;
};

#endif /* GLDRAW_TOOLS_TOOL_H */
