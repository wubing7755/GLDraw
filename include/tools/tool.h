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

/**
 * @brief Normalized pointer/keyboard event payload passed to tools.
 *
 * Contains screen/world positions, deltas, and input state at the moment
 * of an input event. All coordinates are in the relevant space already
 * (screen or world) so tools can use whichever is appropriate.
 */
typedef struct {
    Vec2 screen_pos;   /**< Screen-space cursor position at event time */
    Vec2 world_pos;     /**< World-space cursor position at event time */
    Vec2 delta_screen;  /**< Screen-space movement since last event */
    Vec2 delta_world;   /**< World-space movement since last event */
    int button;         /**< GLFW button index, or -1 for move events */
    int mods;           /**< Modifier key flags at event time */
    float wheel_y;      /**< Vertical scroll delta (0 for non-scroll events) */
} ToolEvent;

/**
 * @brief Shared handles a tool uses to safely read/write editor state.
 *
 * Passed to tool callbacks so tools can access document, canvas, history,
 * and workspace without needing a full workspace reference.
 */
typedef struct {
    struct Workspace* workspace;    /**< Editor state and callbacks */
    struct Document* document;       /**< Active document objects and selection */
    struct DocumentHistory* history; /**< Undo/redo stack */
    struct CanvasView* canvas;       /**< Viewport, zoom, and picking */
} ToolContext;

typedef struct Tool Tool;
typedef struct ToolVTable ToolVTable;

/**
 * @brief Virtual dispatch table for polymorphic tool implementations.
 *
 * Each tool kind provides function pointers for all input handling callbacks.
 * A `NULL` vtable entry means that event type is not supported by that tool.
 */
struct ToolVTable {
    const char* (*label)(const Tool* tool);         /**< Human-readable tool name */
    void (*activate)(Tool* tool, ToolContext* context);       /**< Called when tool becomes active */
    void (*deactivate)(Tool* tool, ToolContext* context);       /**< Called when tool stops being active */
    void (*pointer_down)(Tool* tool, ToolContext* context, const ToolEvent* event);   /**< Left-press handler */
    void (*pointer_move)(Tool* tool, ToolContext* context, const ToolEvent* event);   /**< Pointer move handler */
    void (*pointer_up)(Tool* tool, ToolContext* context, const ToolEvent* event);     /**< Left-release handler */
    void (*key_down)(Tool* tool, ToolContext* context, int key, int mods);           /**< Keyboard handler */
};

/**
 * @brief Runtime instance of a tool with its kind, vtable, and per-tool state.
 */
struct Tool {
    ToolKind kind;                    /**< Which tool kind this instance represents */
    const ToolVTable* vtable;        /**< Dispatch table for this tool's callbacks */
    void* state;                     /**< Per-tool runtime state (e.g., drag state, anchor point) */
    GraphicObject* overlay_object;    /**< Transient preview object currently being drawn */
};

#endif /* GLDRAW_TOOLS_TOOL_H */
