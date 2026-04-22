/**
 * @file workspace.h
 * @brief Shared editor state hub used across runtime systems.
 *
 * Role in project:
 * - Aggregates document, history, canvas, tools, and UI layout state.
 * - Provides function hooks for save/load commands.
 *
 * Module relationships:
 * - Owned by `application.c`.
 * - Referenced by tools and UI for coordinated document operations.
 */
#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <input/keymap.h>
#include <tools/tool_controller.h>

struct Workspace;
/** Callback signature for workspace-level commands (save/load). */
typedef int (*WorkspaceCommandFn)(struct Workspace* workspace, void* user_data);

/** Destructive editor actions that may need unsaved-change confirmation. */
typedef enum WorkspaceActionType {
    WORKSPACE_ACTION_NONE = 0,
    WORKSPACE_ACTION_NEW_DOCUMENT,
    WORKSPACE_ACTION_OPEN_DOCUMENT,
    WORKSPACE_ACTION_EXIT_APPLICATION
} WorkspaceActionType;

/** Top-level UI request kinds owned by the workspace. */
typedef enum UiRequestType {
    UI_REQUEST_NONE = 0,
    UI_REQUEST_DIALOG
} UiRequestType;

/** Stable dialog kinds used to route dialog results back into business state. */
typedef enum UiDialogKind {
    UI_DIALOG_NONE = 0,
    UI_DIALOG_CONFIRM_UNSAVED
} UiDialogKind;

/** Standard dialog button outcomes returned by reusable UI components. */
typedef enum UiDialogResult {
    UI_DIALOG_RESULT_NONE = 0,
    UI_DIALOG_RESULT_PRIMARY,
    UI_DIALOG_RESULT_SECONDARY,
    UI_DIALOG_RESULT_CANCEL
} UiDialogResult;

/** Small reusable payload block reserved for future dialog-specific data. */
typedef struct UiDialogPayload {
    int int_values[4];
    char text[128];
} UiDialogPayload;

/** Static button description used by generic dialog rendering. */
typedef struct UiDialogButtonSpec {
    char label[24];
    UiDialogResult result;
    int is_default;
} UiDialogButtonSpec;

/** Complete dialog state model owned by the workspace and rendered by UI components. */
typedef struct UiDialogState {
    UiDialogKind kind;
    char title[64];
    char message[256];
    UiDialogPayload payload;
    UiDialogButtonSpec buttons[3];
    int button_count;
    float width;
    float height;
    int modal;
} UiDialogState;

/** Callback signature for application-owned workspace action execution. */
typedef int (*WorkspaceActionExecutorFn)(struct Workspace* workspace,
                                         WorkspaceActionType action,
                                         void* user_data);

/** UI-computed layout snapshot shared with input/canvas subsystems. */
typedef struct WorkspaceLayout {
    RectF window_bounds;
    RectF canvas_content_bounds;
    RectF appbar_bounds;
    RectF rail_bounds;
    RectF panel_bounds;
    RectF status_bounds;
    unsigned int layout_revision;
} WorkspaceLayout;

/**
 * @brief Central runtime state passed between systems.
 *
 * Memory ownership notes:
 * - `document`, `history`, `canvas`, and `tools` are embedded values (no extra indirection).
 * - `save_document`/`load_document` are callbacks owned by the application layer.
 *
 * Concurrency note:
 * - This struct is not thread-safe; all access must be serialized by the caller.
 */
typedef struct Workspace {
    Document document;              /**< In-memory document with objects and selection */
    DocumentHistory history;        /**< Undo/redo stacks */
    CanvasView canvas;              /**< Viewport, zoom, pan, and coordinate transforms */
    ToolController tools;           /**< Active tool and its runtime state */
    EditorKeymap keymap;            /**< Effective keyboard mapping state */
    WorkspaceLayout layout;        /**< UI-computed layout rectangles (set by UI system) */
    char current_document_path[260]; /**< Active file path (empty for new documents) */
    char status_message[256];       /**< Current status bar message */
    unsigned int saved_revision;     /**< Document revision last marked as saved */
    int document_dirty;             /**< Non-zero when current revision differs from saved_revision */
    UiRequestType active_request_type; /**< Active UI request type owned by the workspace */
    UiDialogState active_dialog;    /**< Active reusable dialog state */
    WorkspaceCommandFn save_document; /**< Workspace-level save callback */
    WorkspaceCommandFn load_document; /**< Workspace-level load callback */
    WorkspaceActionExecutorFn execute_action; /**< Application-owned action executor */
    void* command_user_data;        /**< Caller-supplied context for command callbacks */
} Workspace;

/**
 * @brief Mark the current document revision as persisted.
 * @param workspace [in,out] Target workspace; ignored when `NULL`.
 * @return None.
 *
 * Edge cases:
 * - Safe no-op for `NULL`.
 *
 * Time complexity: `O(1)`.
 */
static inline void workspace_mark_saved(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->saved_revision = workspace->document.revision;
    workspace->document_dirty = 0;
}

/**
 * @brief Recompute dirty flag from saved revision and current revision.
 * @param workspace [in,out] Target workspace; ignored when `NULL`.
 * @return None.
 *
 * Edge cases:
 * - Safe no-op for `NULL`.
 *
 * Time complexity: `O(1)`.
 */
static inline void workspace_sync_document_dirty(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->document_dirty = (workspace->saved_revision != workspace->document.revision);
}

#endif /* GLDRAW_APP_WORKSPACE_H */
