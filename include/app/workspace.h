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
#include <tools/tool_controller.h>

struct Workspace;
/** Callback signature for workspace-level commands (save/load). */
typedef int (*WorkspaceCommandFn)(struct Workspace* workspace, void* user_data);

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
