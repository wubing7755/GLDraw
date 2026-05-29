/**
 * @file workspace_internal.h
 * @brief Internal workspace state layout for runtime modules and tests.
 *
 * This header intentionally exposes mutable subsystem access for app/runtime
 * implementation files. External UI and integration code should use
 * workspace.h, EditorViewModel, and EditorActionSink instead of depending on
 * this layout.
 */
#ifndef GLDRAW_APP_WORKSPACE_INTERNAL_H
#define GLDRAW_APP_WORKSPACE_INTERNAL_H

#include <app/editor_session.h>
#include <app/workspace.h>
#include <base/path_utils.h>
#include <document/document_internal.h>

typedef struct EditorCore {
    Document document;
    CommandExecutor commands;
    CanvasView canvas;
    ToolController tools;
} EditorCore;

typedef struct EditorSession {
    EditorKeymap keymap;
    WorkspaceLayout layout;
    SelectionSet selection;
    GraphicObject** clipboard_objects;
    int clipboard_count;
    int clipboard_capacity;
    unsigned int clipboard_paste_serial;
    Vec2 selection_preview_delta;
    int selection_preview_active;
    char current_document_path[GLDRAW_PATH_MAX];
    char status_message[256];
    unsigned int saved_revision;
    int document_dirty;
    UiRequestType active_request_type;
    UiDialogState active_dialog;
} EditorSession;

typedef struct EditorServices {
    WorkspaceCommandFn save_document;
    WorkspaceCommandFn save_as_document;
    WorkspaceCommandFn export_png;
    WorkspaceCommandFn load_document;
    WorkspaceActionExecutorFn execute_action;
    void* command_user_data;
} EditorServices;

struct Workspace {
    EditorCore core;
    EditorSession session;
    EditorServices services;
};

Document* workspace_get_document(Workspace* workspace);
CommandExecutor* workspace_get_command_executor(Workspace* workspace);
CanvasView* workspace_get_canvas(Workspace* workspace);
ToolController* workspace_get_tool_controller(Workspace* workspace);
EditorKeymap* workspace_get_keymap(Workspace* workspace);
SelectionSet* workspace_get_selection(Workspace* workspace);
UiDialogState* workspace_get_active_dialog(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_INTERNAL_H */
