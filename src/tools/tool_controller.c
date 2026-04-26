/**
 * @file tool_controller.c
 * @brief Built-in tool implementations and controller dispatch logic.
 *
 * Role in project:
 * - Implements Select/Pan/Line/Rect/Ellipse tools.
 * - Routes pointer/keyboard/scroll events to active tool vtable.
 *
 * Module relationships:
 * - Uses document, history, canvas, and workspace dirty-tracking APIs.
 * - Called by application event callbacks and UI tool rail.
 */
#include <tools/tool_controller.h>

#include <app/workspace.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>

#include <GLFW/glfw3.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

/** Runtime state for select tool drag/document-edit history behavior. */
typedef struct {
    int dragging;
    int moved;
    Vec2 last_world;
    Vec2 drag_delta_total;
    int drag_object_count;
    ObjectId drag_object_ids[DOCUMENT_MAX_SELECTION];
    unsigned int drag_revision_before;
} SelectToolState;

/** Runtime state for pan tool. */
typedef struct {
    int panning;
} PanToolState;

/** Runtime state for shape-creation tools. */
typedef struct {
    int drawing;
    Vec2 anchor;
    Vec2 current;
} ShapeToolState;

/**
 * @brief Resets a snapshot.
 * @param snapshot Snapshot to reset.
 * @return None.
 */
static void snapshot_reset(DocumentSnapshot* snapshot)
{
    document_snapshot_free(snapshot);
    document_snapshot_init(snapshot);
}

/**
 * @brief Destroys a tool's overlay object.
 * @param tool Tool instance.
 * @return None.
 */
static void destroy_overlay(Tool* tool)
{
    if (tool && tool->overlay_object) {
        object_destroy(tool->overlay_object);
        tool->overlay_object = NULL;
    }
}

/**
 * @brief Creates a rectangle from two points.
 * @param a First point.
 * @param b Second point.
 * @return Rectangle bounds.
 */
static RectF rect_from_points(Vec2 a, Vec2 b)
{
    RectF rect;
    rect.x = (a.x < b.x) ? a.x : b.x;
    rect.y = (a.y < b.y) ? a.y : b.y;
    rect.w = fabsf(b.x - a.x);
    rect.h = fabsf(b.y - a.y);
    return rect;
}

/**
 * @brief Builds a shape object from tool kind.
 * @param kind Shape tool kind.
 * @param anchor Anchor point.
 * @param current Current point.
 * @param style Graphic style.
 * @return New shape object or NULL.
 */
static GraphicObject* build_shape_object(ToolKind kind, Vec2 anchor, Vec2 current, GraphicStyle style)
{
    if (kind == TOOL_KIND_LINE) {
        return object_create_line(anchor, current, style);
    }
    if (kind == TOOL_KIND_RECT) {
        return object_create_rect(rect_from_points(anchor, current), style);
    }
    if (kind == TOOL_KIND_ELLIPSE) {
        return object_create_ellipse(rect_from_points(anchor, current), style);
    }
    return NULL;
}

/**
 * @brief Update an existing preview shape object in place.
 * @param object Preview object to update.
 * @param kind Shape tool kind associated with the preview.
 * @param anchor Drag anchor point.
 * @param current Current drag point.
 * @return 1 on success, 0 if the object is missing or does not match the tool kind.
 */
static int update_shape_object(GraphicObject* object, ToolKind kind, Vec2 anchor, Vec2 current)
{
    RectF rect = rect_from_points(anchor, current);

    if (!object) {
        return 0;
    }

    if (kind == TOOL_KIND_LINE && object->type == GRAPHIC_OBJECT_LINE) {
        return object_set_scalar(object, "x1", anchor.x) &&
               object_set_scalar(object, "y1", anchor.y) &&
               object_set_scalar(object, "x2", current.x) &&
               object_set_scalar(object, "y2", current.y);
    }

    if (kind == TOOL_KIND_RECT && object->type == GRAPHIC_OBJECT_RECT) {
        return object_set_scalar(object, "x", rect.x) &&
               object_set_scalar(object, "y", rect.y) &&
               object_set_scalar(object, "width", rect.w) &&
               object_set_scalar(object, "height", rect.h);
    }

    if (kind == TOOL_KIND_ELLIPSE && object->type == GRAPHIC_OBJECT_ELLIPSE) {
        return object_set_scalar(object, "x", rect.x) &&
               object_set_scalar(object, "y", rect.y) &&
               object_set_scalar(object, "width", rect.w) &&
               object_set_scalar(object, "height", rect.h);
    }

    return 0;
}

/**
 * @brief Pushes document edit to history and syncs dirty flag.
 * @param context [in,out] Tool context.
 * @param before_snapshot [in,out] Before snapshot (consumed and reset by this function).
 * @return None.
 *
 * @remark Safely resets snapshot even when context/history is invalid.
 */
static void tool_commit_document_change(ToolContext* context,
                                        DocumentSnapshot* before_snapshot,
                                        const SelectionSet* before_selection)
{
    if (!context || !context->history || !before_snapshot || !before_selection || !context->selection) {
        if (before_snapshot) {
            snapshot_reset(before_snapshot);
        }
        return;
    }

    if (!document_history_push(context->history,
                               before_snapshot,
                               before_selection,
                               context->document,
                               context->selection)) {
        snapshot_reset(before_snapshot);
        return;
    }

    workspace_sync_document_dirty(context->workspace);
}

/**
 * @brief Gets the select tool label.
 * @param tool Tool instance.
 * @return Tool label string.
 */
static const char* select_tool_label(const Tool* tool)
{
    (void)tool;
    return "Select";
}

/**
 * @brief Deactivates the select tool.
 * @param tool Tool instance.
 * @param context Tool context.
 * @return None.
 */
static void select_tool_deactivate(Tool* tool, ToolContext* context)
{
    SelectToolState* state = (SelectToolState*)tool->state;
    (void)context;
    state->dragging = 0;
    state->moved = 0;
    state->drag_delta_total = vec2_make(0.0f, 0.0f);
    state->drag_object_count = 0;
    state->drag_revision_before = 0u;
}

/**
 * @brief Handles select tool pointer press.
 * @param tool [in,out] Select tool instance.
 * @param context [in,out] Tool context.
 * @param event [in] Pointer event.
 * @return None.
 */
static int select_tool_pointer_down(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    SelectToolState* state = (SelectToolState*)tool->state;
    GraphicObject* hit = NULL;
    int i = 0;

    if (event->button != GLFW_MOUSE_BUTTON_LEFT) {
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

/**
 * @brief Moves selected objects while dragging.
 * @param tool Tool instance.
 * @param context Tool context.
 * @param event Pointer event.
 * @return None.
 */
static void select_tool_pointer_move(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    SelectToolState* state = (SelectToolState*)tool->state;
    Vec2 delta = {0.0f, 0.0f};
    int i = 0;

    if (!state->dragging) {
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

/**
 * @brief Finalizes select drag handling and commits history when needed.
 * @param tool Tool instance.
 * @param context Tool context.
 * @param event Pointer event.
 * @return None.
 */
static void select_tool_pointer_up(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    SelectToolState* state = (SelectToolState*)tool->state;
    (void)event;

    if (state->dragging &&
        state->moved &&
        state->drag_object_count > 0 &&
        vec2_length_sq(state->drag_delta_total) > 1e-6f &&
        context &&
        context->history) {
        document_history_push_translate_edit(context->history,
                                             context->document,
                                             context->selection,
                                             state->drag_object_ids,
                                             state->drag_object_count,
                                             state->drag_delta_total,
                                             state->drag_revision_before,
                                             context->document->revision);
        workspace_sync_document_dirty(context->workspace);
    }

    state->dragging = 0;
    state->moved = 0;
    state->drag_delta_total = vec2_make(0.0f, 0.0f);
    state->drag_object_count = 0;
    state->drag_revision_before = 0u;
}

/**
 * @brief Handles select tool keyboard input.
 * @param tool Tool instance.
 * @param context Tool context.
 * @param key GLFW key code.
 * @param mods Modifier flags.
 * @return None.
 */
static void select_tool_key_down(Tool* tool, ToolContext* context, int key, int mods)
{
    (void)tool;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE) {
        selection_set_clear(context->selection);
    }
}

static const ToolVTable g_select_tool_vtable = {
    select_tool_label,
    NULL,
    select_tool_deactivate,
    select_tool_pointer_down,
    select_tool_pointer_move,
    select_tool_pointer_up,
    select_tool_key_down
};

/**
 * @brief Gets the pan tool label.
 * @param tool Tool instance.
 * @return Tool label string.
 */
static const char* pan_tool_label(const Tool* tool)
{
    (void)tool;
    return "Hand";
}

/**
 * @brief Deactivates the pan tool.
 * @param tool Tool instance.
 * @param context Tool context.
 * @return None.
 */
static void pan_tool_deactivate(Tool* tool, ToolContext* context)
{
    PanToolState* state = (PanToolState*)tool->state;
    (void)context;
    state->panning = 0;
}

/**
 * @brief Handles pan tool pointer press.
 * @param tool Tool instance.
 * @param context Tool context.
 * @param event Pointer event.
 * @return Non-zero when the pan tool accepted the interaction.
 */
static int pan_tool_pointer_down(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    PanToolState* state = (PanToolState*)tool->state;
    (void)tool;
    (void)context;
    if (event->button != GLFW_MOUSE_BUTTON_LEFT) {
        return 0;
    }
    state->panning = 1;
    return 1;
}

/**
 * @brief Handles pan tool pointer movement.
 * @param tool Tool instance.
 * @param context Tool context.
 * @param event Pointer event.
 * @return None.
 */
static void pan_tool_pointer_move(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    PanToolState* state = (PanToolState*)tool->state;
    (void)tool;
    if (!state->panning) {
        return;
    }
    canvas_view_pan_screen_delta(context->canvas, event->delta_screen);
}

/**
 * @brief Handles pan tool pointer release.
 * @param tool Tool instance.
 * @param context Tool context.
 * @param event Pointer event.
 * @return None.
 */
static void pan_tool_pointer_up(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    PanToolState* state = (PanToolState*)tool->state;
    (void)context;
    (void)event;
    if (state->panning) {
        state->panning = 0;
    }
}

/**
 * @brief Handles pan tool keyboard input.
 * @param tool Tool instance.
 * @param context Tool context.
 * @param key GLFW key code.
 * @param mods Modifier flags.
 * @return None.
 */
static void pan_tool_key_down(Tool* tool, ToolContext* context, int key, int mods)
{
    PanToolState* state = (PanToolState*)tool->state;
    (void)context;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE && state->panning) {
        state->panning = 0;
    }
}

static const ToolVTable g_pan_tool_vtable = {
    pan_tool_label,
    NULL,
    pan_tool_deactivate,
    pan_tool_pointer_down,
    pan_tool_pointer_move,
    pan_tool_pointer_up,
    pan_tool_key_down
};

/** Return label by shape kind. */
static const char* shape_tool_label(const Tool* tool)
{
    switch (tool->kind) {
    case TOOL_KIND_LINE:
        return "Line";
    case TOOL_KIND_RECT:
        return "Rectangle";
    case TOOL_KIND_ELLIPSE:
        return "Ellipse";
    default:
        return "Shape";
    }
}

/** Cancel shape drawing and clear overlay when tool deactivates. */
static void shape_tool_deactivate(Tool* tool, ToolContext* context)
{
    ShapeToolState* state = (ShapeToolState*)tool->state;
    (void)context;
    state->drawing = 0;
    destroy_overlay(tool);
}

/** Start shape draw interaction and create translucent overlay preview. */
static int shape_tool_pointer_down(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    ShapeToolState* state = (ShapeToolState*)tool->state;
    GraphicStyle style = object_default_style();

    (void)context;
    if (event->button != GLFW_MOUSE_BUTTON_LEFT) {
        return 0;
    }

    state->drawing = 1;
    state->anchor = event->world_pos;
    state->current = event->world_pos;

    style.stroke_color.a = 0.75f;
    destroy_overlay(tool);
    tool->overlay_object = build_shape_object(tool->kind, state->anchor, state->current, style);
    return (tool->overlay_object != NULL);
}

/** Update shape overlay as pointer moves. */
static void shape_tool_pointer_move(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    ShapeToolState* state = (ShapeToolState*)tool->state;

    (void)context;
    if (!state->drawing) {
        return;
    }

    state->current = event->world_pos;
    if (!update_shape_object(tool->overlay_object, tool->kind, state->anchor, state->current)) {
        destroy_overlay(tool);
        state->drawing = 0;
    }
}

/**
 * @brief Finalize shape creation on pointer release.
 * Risk note:
 * - Captures history snapshot before append; allocation/add failures clean up
 *   object/snapshot to avoid leaks and partial commits.
 */
static void shape_tool_pointer_up(Tool* tool, ToolContext* context, const ToolEvent* event)
{
    ShapeToolState* state = (ShapeToolState*)tool->state;
    GraphicStyle style = object_default_style();
    GraphicObject* object = NULL;
    DocumentSnapshot before_snapshot;
    SelectionSet before_selection;

    (void)event;
    if (!state->drawing) {
        return;
    }

    document_snapshot_init(&before_snapshot);
    before_selection = *context->selection;
    document_snapshot_capture(&before_snapshot, context->document);
    state->drawing = 0;
    object = build_shape_object(tool->kind, state->anchor, state->current, style);
    destroy_overlay(tool);

    if (!object) {
        document_snapshot_free(&before_snapshot);
        return;
    }

    if (!document_add_object(context->document, object)) {
        object_destroy(object);
        document_snapshot_free(&before_snapshot);
        return;
    }

    selection_set_clear(context->selection);
    selection_set_add(context->selection, object->id);
    tool_commit_document_change(context, &before_snapshot, &before_selection);
}

/** Escape cancels in-progress shape drawing. */
static void shape_tool_key_down(Tool* tool, ToolContext* context, int key, int mods)
{
    (void)mods;
    if (key == GLFW_KEY_ESCAPE) {
        shape_tool_deactivate(tool, context);
    }
}

static const ToolVTable g_shape_tool_vtable = {
    shape_tool_label,
    NULL,
    shape_tool_deactivate,
    shape_tool_pointer_down,
    shape_tool_pointer_move,
    shape_tool_pointer_up,
    shape_tool_key_down
};

/** Initialize one tool slot and allocate optional state block. */
static void tool_init_slot(Tool* tool, ToolKind kind, const ToolVTable* vtable, size_t state_size)
{
    memset(tool, 0, sizeof(*tool));
    tool->kind = kind;
    tool->vtable = vtable;
    if (state_size > 0) {
        tool->state = calloc(1, state_size);
    }
}

/** Initialize all built-in tools and default active tool. */
void tool_controller_init(ToolController* controller)
{
    if (!controller) {
        return;
    }

    memset(controller, 0, sizeof(*controller));
    tool_init_slot(&controller->tools[TOOL_KIND_SELECT], TOOL_KIND_SELECT, &g_select_tool_vtable, sizeof(SelectToolState));
    tool_init_slot(&controller->tools[TOOL_KIND_PAN], TOOL_KIND_PAN, &g_pan_tool_vtable, sizeof(PanToolState));
    tool_init_slot(&controller->tools[TOOL_KIND_LINE], TOOL_KIND_LINE, &g_shape_tool_vtable, sizeof(ShapeToolState));
    tool_init_slot(&controller->tools[TOOL_KIND_RECT], TOOL_KIND_RECT, &g_shape_tool_vtable, sizeof(ShapeToolState));
    tool_init_slot(&controller->tools[TOOL_KIND_ELLIPSE], TOOL_KIND_ELLIPSE, &g_shape_tool_vtable, sizeof(ShapeToolState));
    controller->active_kind = TOOL_KIND_SELECT;
}

/** Shutdown all tool states and overlays. */
void tool_controller_shutdown(ToolController* controller)
{
    int i = 0;

    if (!controller) {
        return;
    }

    for (i = 0; i < TOOL_KIND_COUNT; ++i) {
        destroy_overlay(&controller->tools[i]);
        free(controller->tools[i].state);
        controller->tools[i].state = NULL;
    }
}

/** Return active tool pointer. */
Tool* tool_controller_get_active(ToolController* controller)
{
    if (!controller) {
        return NULL;
    }
    return &controller->tools[controller->active_kind];
}

/** Return active tool label. */
const char* tool_controller_active_label(const ToolController* controller)
{
    const Tool* tool = NULL;
    if (!controller) {
        return "Unknown";
    }
    tool = &controller->tools[controller->active_kind];
    return tool->vtable->label(tool);
}

/** Return active tool overlay object if any. */
GraphicObject* tool_controller_overlay_object(const ToolController* controller)
{
    if (!controller) {
        return NULL;
    }
    return controller->tools[controller->active_kind].overlay_object;
}

/** Switch active tool with proper deactivate/activate callbacks. */
void tool_controller_set_active(ToolController* controller, ToolContext* context, ToolKind kind)
{
    Tool* current = NULL;
    Tool* next = NULL;

    if (!controller || kind < 0 || kind >= TOOL_KIND_COUNT || controller->active_kind == kind) {
        return;
    }

    current = tool_controller_get_active(controller);
    if (current && current->vtable && current->vtable->deactivate) {
        current->vtable->deactivate(current, context);
    }

    controller->active_kind = kind;
    next = tool_controller_get_active(controller);
    if (next && next->vtable && next->vtable->activate) {
        next->vtable->activate(next, context);
    }
}

/** Dispatch pointer-down and capture pointer. */
void tool_controller_pointer_down(ToolController* controller, ToolContext* context, const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    int accepted = 0;

    if (!controller || !tool || !tool->vtable || !tool->vtable->pointer_down) {
        return;
    }

    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
    accepted = tool->vtable->pointer_down(tool, context, event);
    controller->pointer_captured = accepted ? 1 : 0;
}

/** Dispatch pointer-move to active tool. */
void tool_controller_pointer_move(ToolController* controller, ToolContext* context, const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    if (!controller || !tool || !tool->vtable || !tool->vtable->pointer_move) {
        return;
    }
    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
    tool->vtable->pointer_move(tool, context, event);
}

/** Dispatch pointer-up and release pointer capture. */
void tool_controller_pointer_up(ToolController* controller, ToolContext* context, const ToolEvent* event)
{
    Tool* tool = tool_controller_get_active(controller);
    if (!controller || !tool || !tool->vtable || !tool->vtable->pointer_up) {
        return;
    }
    tool->vtable->pointer_up(tool, context, event);
    controller->pointer_captured = 0;
    controller->last_screen = event->screen_pos;
    controller->last_world = event->world_pos;
}

/** Dispatch key-down to the active tool only. */
void tool_controller_key_down(ToolController* controller, ToolContext* context, int key, int mods)
{
    Tool* tool = NULL;

    if (!controller) {
        return;
    }

    tool = tool_controller_get_active(controller);
    if (tool && tool->vtable && tool->vtable->key_down) {
        tool->vtable->key_down(tool, context, key, mods);
    }
}

/** Apply wheel zoom around current cursor screen point. */
void tool_controller_scroll(ToolController* controller, ToolContext* context, Vec2 screen_pos, float yoffset)
{
    float factor = 1.0f;
    (void)controller;

    if (!context || !context->canvas || yoffset == 0.0f) {
        return;
    }

    factor = (yoffset > 0.0f) ? 1.1f : 0.9f;
    canvas_view_zoom_at_screen_point(context->canvas, factor, screen_pos);
}
