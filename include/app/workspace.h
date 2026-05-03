/**
 * @file workspace.h
 * @brief Editor runtime shared workspace definition.
 */
#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <app/editor_session.h>
#include <base/path_utils.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
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
    UI_DIALOG_CONFIRM_UNSAVED,
    UI_DIALOG_SHORTCUTS,
    UI_DIALOG_INFO,
    UI_DIALOG_SAVE_AS
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
    char text[1024];
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
    char message[1024];
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
 * @struct EditorCore
 * @brief Backend-agnostic editing state.
 *
 * @member document Persisted document model.
 * @member history Undo/redo history.
 * @member canvas Canvas view state.
 * @member tools Tool controller.
 */
typedef struct EditorCore {
    Document document;
    DocumentHistory history;
    CommandExecutor commands;
    CanvasView canvas;
    ToolController tools;
} EditorCore;

/**
 * @struct EditorSession
 * @brief Session-only editor state.
 *
 * @member keymap Effective keyboard mapping state.
 * @member layout UI layout information.
 * @member selection Active object selection.
 * @member clipboard_objects Internal clipboard object clones.
 * @member clipboard_count Number of objects currently stored in the internal clipboard.
 * @member clipboard_paste_serial Repeated paste counter used to offset pasted content.
 * @member current_document_path Current document path (empty string means unnamed).
 * @member status_message Status bar message.
 * @member saved_revision Document revision corresponding to the last save.
 * @member document_dirty Whether the document is dirty (non-zero means unsaved changes).
 * @member active_request_type Active top-level UI request type.
 * @member active_dialog Active reusable dialog state.
 */
typedef struct EditorSession {
    EditorKeymap keymap;
    WorkspaceLayout layout;
    SelectionSet selection;
    GraphicObject* clipboard_objects[DOCUMENT_MAX_SELECTION];
    int clipboard_count;
    unsigned int clipboard_paste_serial;
    char current_document_path[GLDRAW_PATH_MAX];
    char status_message[256];
    unsigned int saved_revision;
    int document_dirty;
    UiRequestType active_request_type;
    UiDialogState active_dialog;
} EditorSession;

/**
 * @struct EditorServices
 * @brief Integration callbacks supplied by the application shell.
 */
typedef struct EditorServices {
    WorkspaceCommandFn save_document;
    WorkspaceCommandFn save_as_document;
    WorkspaceCommandFn export_png;
    WorkspaceCommandFn load_document;
    WorkspaceActionExecutorFn execute_action;
    void* command_user_data;
} EditorServices;

/**
 * @struct Workspace
 * @brief Runtime editor container split into core/session/services layers.
 */
typedef struct Workspace {
    EditorCore core;
    EditorSession session;
    EditorServices services;
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

    workspace->session.saved_revision = workspace->core.document.revision;
    workspace->session.document_dirty = 0;
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

    workspace->session.document_dirty =
        (workspace->session.saved_revision != workspace->core.document.revision);
}

/**
 * @brief Clear all objects stored in the internal clipboard.
 * @param workspace Target workspace; no-op if `NULL`.
 * @return No return value.
 */
static inline void workspace_clear_clipboard(Workspace* workspace)
{
    int i = 0;

    if (!workspace) {
        return;
    }

    for (i = 0; i < workspace->session.clipboard_count; ++i) {
        object_destroy(workspace->session.clipboard_objects[i]);
        workspace->session.clipboard_objects[i] = NULL;
    }

    workspace->session.clipboard_count = 0;
    workspace->session.clipboard_paste_serial = 0;
}

#endif /* GLDRAW_APP_WORKSPACE_H */
