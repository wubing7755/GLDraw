/**
 * @file workspace.h
 * @brief Editor runtime shared workspace definition.
 */
#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
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
 * @member layout UI layout information.
 * @member current_document_path Current document path (empty string means unnamed).
 * @member status_message Status bar message.
 * @member saved_revision Document revision corresponding to the last save.
 * @member document_dirty Whether the document is dirty (non-zero means unsaved changes).
 * @member save_document Save callback.
 * @member load_document Load callback.
 * @member command_user_data Callback context.
 */
typedef struct Workspace {
    Document document;
    DocumentHistory history;
    CanvasView canvas;
    ToolController tools;
    WorkspaceLayout layout;
    char current_document_path[260];
    char status_message[256];
    unsigned int saved_revision;
    int document_dirty;
    WorkspaceCommandFn save_document;
    WorkspaceCommandFn load_document;
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
