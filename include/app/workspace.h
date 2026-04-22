/**
 * @file workspace.h
 * @brief Editor runtime shared workspace definition.
 */
#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <input/keymap.h>
#include <tools/tool_controller.h>

struct Workspace;

/**
 * @typedef WorkspaceCommandFn
 * @brief Workspace command callback signature (e.g., save/load).
 * @param workspace Target workspace.
 * @param user_data Caller-supplied context passed through.
 * @return Non-zero on success, zero on failure.
 */
typedef int (*WorkspaceCommandFn)(struct Workspace* workspace, void* user_data);

/**
 * @enum WorkspaceActionType
 * @brief Destructive workspace actions that may require deferred confirmation.
 */
typedef enum WorkspaceActionType {
    WORKSPACE_ACTION_NONE = 0,
    WORKSPACE_ACTION_NEW_DOCUMENT,
    WORKSPACE_ACTION_OPEN_DOCUMENT,
    WORKSPACE_ACTION_EXIT_APPLICATION
} WorkspaceActionType;

/**
 * @enum UiRequestType
 * @brief Top-level UI request kinds owned by the workspace.
 */
typedef enum UiRequestType {
    UI_REQUEST_NONE = 0,
    UI_REQUEST_DIALOG
} UiRequestType;

/**
 * @enum UiDialogKind
 * @brief Stable dialog kinds used to resolve reusable dialog flows.
 */
typedef enum UiDialogKind {
    UI_DIALOG_NONE = 0,
    UI_DIALOG_CONFIRM_UNSAVED
} UiDialogKind;

/**
 * @enum UiDialogResult
 * @brief Standard dialog outcomes returned by reusable UI components.
 */
typedef enum UiDialogResult {
    UI_DIALOG_RESULT_NONE = 0,
    UI_DIALOG_RESULT_PRIMARY,
    UI_DIALOG_RESULT_SECONDARY,
    UI_DIALOG_RESULT_CANCEL
} UiDialogResult;

/**
 * @struct UiDialogPayload
 * @brief Small reusable payload block reserved for future dialog-specific data.
 *
 * @member int_values Integer payload slots.
 * @member text Text payload buffer.
 */
typedef struct UiDialogPayload {
    int int_values[4];
    char text[128];
} UiDialogPayload;

/**
 * @struct UiDialogButtonSpec
 * @brief Static button description used by generic dialog rendering.
 *
 * @member label Button label text.
 * @member result Result emitted when the button is selected.
 * @member is_default Non-zero when this button is the default action.
 */
typedef struct UiDialogButtonSpec {
    char label[24];
    UiDialogResult result;
    int is_default;
} UiDialogButtonSpec;

/**
 * @struct UiDialogState
 * @brief Complete dialog state model owned by the workspace and rendered by UI components.
 *
 * @member kind Stable dialog kind used by business logic.
 * @member title Dialog title.
 * @member message Dialog body text.
 * @member payload Reserved dialog payload.
 * @member buttons Button specifications.
 * @member button_count Number of active buttons in `buttons`.
 * @member width Preferred dialog width.
 * @member height Preferred dialog height.
 * @member modal Non-zero when the dialog blocks background interaction.
 */
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

/**
 * @typedef WorkspaceActionExecutorFn
 * @brief Application-owned executor for deferred workspace actions.
 * @param workspace Target workspace.
 * @param action Requested action.
 * @param user_data Caller-supplied context passed through.
 * @return Non-zero on success, zero on failure.
 */
typedef int (*WorkspaceActionExecutorFn)(struct Workspace* workspace,
                                         WorkspaceActionType action,
                                         void* user_data);

/**
 * @struct WorkspaceLayout
 * @brief Layout snapshot computed by the UI.
 *
 * @member window_bounds Window boundary.
 * @member canvas_content_bounds Available canvas content area.
 * @member appbar_bounds Top toolbar area.
 * @member rail_bounds Left tool rail area.
 * @member panel_bounds Right panel area.
 * @member status_bounds Bottom status bar area.
 * @member layout_revision Layout version number (used for cross-system sync).
 */
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
 * @struct Workspace
 * @brief Runtime core state container.
 *
 * @member document Current document object and selection state.
 * @member history Undo/redo history.
 * @member canvas Canvas view state.
 * @member tools Tool controller.
 * @member keymap Effective keyboard mapping state.
 * @member layout UI layout information.
 * @member current_document_path Current document path (empty string means unnamed).
 * @member status_message Status bar message.
 * @member saved_revision Document revision corresponding to the last save.
 * @member document_dirty Whether the document is dirty (non-zero means unsaved changes).
 * @member active_request_type Active top-level UI request type.
 * @member active_dialog Active reusable dialog state.
 * @member save_document Save callback.
 * @member load_document Load callback.
 * @member execute_action Deferred workspace action executor.
 * @member command_user_data Callback context.
 */
typedef struct Workspace {
    Document document;
    DocumentHistory history;
    CanvasView canvas;
    ToolController tools;
    EditorKeymap keymap;
    WorkspaceLayout layout;
    char current_document_path[260];
    char status_message[256];
    unsigned int saved_revision;
    int document_dirty;
    UiRequestType active_request_type;
    UiDialogState active_dialog;
    WorkspaceCommandFn save_document;
    WorkspaceCommandFn load_document;
    WorkspaceActionExecutorFn execute_action;
    void* command_user_data;
} Workspace;

/**
 * @brief Mark the current document revision as saved.
 * @param workspace Target workspace; no-op if `NULL`.
 * @return No return value.
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
 * @brief Sync the dirty flag based on `saved_revision` and the current revision.
 * @param workspace Target workspace; no-op if `NULL`.
 * @return No return value.
 */
static inline void workspace_sync_document_dirty(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->document_dirty = (workspace->saved_revision != workspace->document.revision);
}

#endif /* GLDRAW_APP_WORKSPACE_H */
