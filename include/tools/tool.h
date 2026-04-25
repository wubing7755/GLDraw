/**
 * @file tool.h
 * @brief Tool system abstract interface and event data structures.
 */
#ifndef GLDRAW_TOOLS_TOOL_H
#define GLDRAW_TOOLS_TOOL_H

#include <base/types.h>
#include <document/object.h>

struct Workspace;
struct Document;
struct CanvasView;
struct DocumentHistory;

/**
 * @enum ToolKind
 * @brief Built-in tool types.
 */
typedef enum {
    TOOL_KIND_SELECT = 0,
    TOOL_KIND_PAN,
    TOOL_KIND_LINE,
    TOOL_KIND_RECT,
    TOOL_KIND_ELLIPSE,
    TOOL_KIND_COUNT
} ToolKind;

/**
 * @struct ToolEvent
 * @brief Normalized input event passed to tools.
 *
 * @member screen_pos Current screen coordinates.
 * @member world_pos Current world coordinates.
 * @member delta_screen Screen-space displacement since the last event.
 * @member delta_world World-space displacement since the last event.
 * @member button Mouse button number (-1 for move events).
 * @member mods Modifier key bitmask.
 * @member wheel_y Scroll wheel Y delta.
 */
typedef struct {
    Vec2 screen_pos;
    Vec2 world_pos;
    Vec2 delta_screen;
    Vec2 delta_world;
    int button;
    int mods;
    float wheel_y;
} ToolEvent;

/**
 * @struct ToolContext
 * @brief Tool operation context.
 *
 * @member workspace Workspace.
 * @member document Current document.
 * @member history History stack.
 * @member canvas Canvas view.
 */
typedef struct {
    struct Workspace* workspace;
    struct Document* document;
    struct DocumentHistory* history;
    struct CanvasView* canvas;
} ToolContext;

typedef struct Tool Tool;
typedef struct ToolVTable ToolVTable;

/**
 * @struct ToolVTable
 * @brief Tool polymorphic callback table.
 *
 * @member label Returns the tool display name.
 * @member activate Tool activate callback.
 * @member deactivate Tool deactivate callback.
 * @member pointer_down Mouse pointer down callback. Returns non-zero only when
 * the tool accepts the interaction and expects captured follow-up events.
 * @member pointer_move Mouse pointer move callback.
 * @member pointer_up Mouse pointer up callback.
 * @member key_down Key down callback.
 */
struct ToolVTable {
    const char* (*label)(const Tool* tool);
    void (*activate)(Tool* tool, ToolContext* context);
    void (*deactivate)(Tool* tool, ToolContext* context);
    int (*pointer_down)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*pointer_move)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*pointer_up)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*key_down)(Tool* tool, ToolContext* context, int key, int mods);
};

/**
 * @struct Tool
 * @brief Tool runtime instance.
 *
 * @member kind Tool type.
 * @member vtable Callback table.
 * @member state Tool-private state.
 * @member overlay_object Tool preview graphic object.
 */
struct Tool {
    ToolKind kind;
    const ToolVTable* vtable;
    void* state;
    GraphicObject* overlay_object;
};

#endif /* GLDRAW_TOOLS_TOOL_H */
