#include <app/workspace_internal.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int workspace_tool_execute_command(void* user, Command* command, Document* document)
{
    Workspace* workspace = (Workspace*)user;

    if (!workspace || !command || !document) {
        return 0;
    }

    return command_executor_execute(&workspace->core.commands, command, document);
}

static int workspace_tool_undo(void* user, Document* document)
{
    Workspace* workspace = (Workspace*)user;

    if (!workspace || !document) {
        return 0;
    }

    return command_executor_undo(&workspace->core.commands, document);
}

static int workspace_tool_redo(void* user, Document* document)
{
    Workspace* workspace = (Workspace*)user;

    if (!workspace || !document) {
        return 0;
    }

    return command_executor_redo(&workspace->core.commands, document);
}

static void workspace_tool_set_selection_preview(void* user, int active, Vec2 delta)
{
    Workspace* workspace = (Workspace*)user;

    if (!workspace) {
        return;
    }

    workspace->session.selection_preview_active = active;
    workspace->session.selection_preview_delta = active ? delta : (Vec2){0.0f, 0.0f};
}

static void workspace_tool_sync_document_dirty(void* user)
{
    workspace_sync_document_dirty((Workspace*)user);
}

ToolContext workspace_tool_context(Workspace* workspace)
{
    ToolContext context;

    context.document = workspace ? &workspace->core.document : NULL;
    context.canvas = workspace ? &workspace->core.canvas : NULL;
    context.selection = workspace ? &workspace->session.selection : NULL;
    context.ports.execute_command = workspace ? workspace_tool_execute_command : NULL;
    context.ports.undo = workspace ? workspace_tool_undo : NULL;
    context.ports.redo = workspace ? workspace_tool_redo : NULL;
    context.ports.set_selection_preview = workspace ? workspace_tool_set_selection_preview : NULL;
    context.ports.sync_document_dirty = workspace ? workspace_tool_sync_document_dirty : NULL;
    context.ports.user = workspace;
    return context;
}

Document* workspace_get_document(Workspace* workspace)
{
    return workspace ? &workspace->core.document : NULL;
}

CommandExecutor* workspace_get_command_executor(Workspace* workspace)
{
    return workspace ? &workspace->core.commands : NULL;
}

CanvasView* workspace_get_canvas(Workspace* workspace)
{
    return workspace ? &workspace->core.canvas : NULL;
}

ToolController* workspace_get_tool_controller(Workspace* workspace)
{
    return workspace ? &workspace->core.tools : NULL;
}

EditorKeymap* workspace_get_keymap(Workspace* workspace)
{
    return workspace ? &workspace->session.keymap : NULL;
}

SelectionSet* workspace_get_selection(Workspace* workspace)
{
    return workspace ? &workspace->session.selection : NULL;
}

const char* workspace_get_status_message(const Workspace* workspace)
{
    return workspace ? workspace->session.status_message : "";
}

WorkspaceLayout workspace_get_layout(const Workspace* workspace)
{
    return workspace ? workspace->session.layout : (WorkspaceLayout){0};
}

UiRequestType workspace_get_active_request_type(const Workspace* workspace)
{
    return workspace ? workspace->session.active_request_type : UI_REQUEST_NONE;
}

UiDialogState* workspace_get_active_dialog(Workspace* workspace)
{
    return workspace ? &workspace->session.active_dialog : NULL;
}

const UiDialogState* workspace_get_active_dialog_const(const Workspace* workspace)
{
    return workspace ? &workspace->session.active_dialog : NULL;
}

int workspace_document_dirty(const Workspace* workspace)
{
    return workspace ? workspace->session.document_dirty : 0;
}

unsigned int workspace_saved_revision(const Workspace* workspace)
{
    return workspace ? workspace->session.saved_revision : 0u;
}

const char* workspace_get_current_document_path(const Workspace* workspace)
{
    return workspace ? workspace->session.current_document_path : "";
}

void workspace_set_status_message(Workspace* workspace, const char* message)
{
    if (!workspace) {
        return;
    }

    snprintf(workspace->session.status_message,
             sizeof(workspace->session.status_message),
             "%s",
             message ? message : "");
}

void workspace_set_statusf(Workspace* workspace, const char* fmt, ...)
{
    va_list args;

    if (!workspace) {
        return;
    }
    if (!fmt) {
        workspace_set_status_message(workspace, "");
        return;
    }

    va_start(args, fmt);
    vsnprintf(workspace->session.status_message,
              sizeof(workspace->session.status_message),
              fmt,
              args);
    va_end(args);
}

void workspace_set_layout(Workspace* workspace, WorkspaceLayout layout)
{
    if (workspace) {
        workspace->session.layout = layout;
    }
}

int workspace_selection_preview_active(const Workspace* workspace)
{
    return workspace ? workspace->session.selection_preview_active : 0;
}

Vec2 workspace_selection_preview_delta(const Workspace* workspace)
{
    return workspace ? workspace->session.selection_preview_delta
                     : (Vec2){0.0f, 0.0f};
}

void workspace_mark_saved(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->session.saved_revision = workspace->core.document.revision;
    workspace->session.document_dirty = 0;
}

void workspace_sync_document_dirty(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->session.document_dirty =
        (workspace->session.saved_revision != workspace->core.document.revision);
}

void workspace_clear_clipboard(Workspace* workspace)
{
    int i = 0;

    if (!workspace) {
        return;
    }

    for (i = 0; i < workspace->session.clipboard_count; ++i) {
        object_destroy(workspace->session.clipboard_objects[i]);
    }

    free(workspace->session.clipboard_objects);
    workspace->session.clipboard_objects = NULL;
    workspace->session.clipboard_count = 0;
    workspace->session.clipboard_capacity = 0;
    workspace->session.clipboard_paste_serial = 0;
}
