#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <tools/tool_controller.h>

struct Workspace;
typedef int (*WorkspaceCommandFn)(struct Workspace* workspace, void* user_data);

typedef struct Workspace {
    Document document;
    DocumentHistory history;
    CanvasView canvas;
    ToolController tools;
    char current_document_path[260];
    unsigned int saved_revision;
    int document_dirty;
    WorkspaceCommandFn save_document;
    WorkspaceCommandFn load_document;
    void* command_user_data;
} Workspace;

static inline void workspace_mark_saved(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->saved_revision = workspace->document.revision;
    workspace->document_dirty = 0;
}

static inline void workspace_sync_document_dirty(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->document_dirty = (workspace->saved_revision != workspace->document.revision);
}

#endif /* GLDRAW_APP_WORKSPACE_H */
