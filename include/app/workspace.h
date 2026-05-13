/**
 * @file workspace.h
 * @brief Public workspace types and opaque workspace API.
 */
#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <base/types.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
#include <input/keymap.h>
#include <model/selection.h>
#include <tools/tool.h>
#include <tools/tool_controller.h>

typedef struct Workspace Workspace;

typedef int (*WorkspaceCommandFn)(Workspace* workspace, void* user_data);

typedef enum WorkspaceActionType {
    WORKSPACE_ACTION_NONE = 0,
    WORKSPACE_ACTION_NEW_DOCUMENT,
    WORKSPACE_ACTION_OPEN_DOCUMENT,
    WORKSPACE_ACTION_EXIT_APPLICATION
} WorkspaceActionType;

typedef enum UiRequestType {
    UI_REQUEST_NONE = 0,
    UI_REQUEST_DIALOG
} UiRequestType;

typedef enum UiDialogKind {
    UI_DIALOG_NONE = 0,
    UI_DIALOG_CONFIRM_UNSAVED,
    UI_DIALOG_SHORTCUTS,
    UI_DIALOG_INFO,
    UI_DIALOG_SAVE_AS
} UiDialogKind;

typedef enum UiDialogResult {
    UI_DIALOG_RESULT_NONE = 0,
    UI_DIALOG_RESULT_PRIMARY,
    UI_DIALOG_RESULT_SECONDARY,
    UI_DIALOG_RESULT_CANCEL
} UiDialogResult;

typedef struct UiDialogPayload {
    int int_values[4];
    char text[1024];
} UiDialogPayload;

typedef struct UiDialogButtonSpec {
    char label[24];
    UiDialogResult result;
    int is_default;
} UiDialogButtonSpec;

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

typedef int (*WorkspaceActionExecutorFn)(Workspace* workspace,
                                         WorkspaceActionType action,
                                         void* user_data);

typedef struct WorkspaceLayout {
    RectF window_bounds;
    RectF canvas_content_bounds;
    RectF appbar_bounds;
    RectF rail_bounds;
    RectF panel_bounds;
    RectF status_bounds;
    unsigned int layout_revision;
} WorkspaceLayout;

ToolContext workspace_tool_context(Workspace* workspace);
Document* workspace_get_document(Workspace* workspace);
CommandExecutor* workspace_get_command_executor(Workspace* workspace);
CanvasView* workspace_get_canvas(Workspace* workspace);
ToolController* workspace_get_tool_controller(Workspace* workspace);
EditorKeymap* workspace_get_keymap(Workspace* workspace);
SelectionSet* workspace_get_selection(Workspace* workspace);
const char* workspace_get_status_message(const Workspace* workspace);
WorkspaceLayout workspace_get_layout(const Workspace* workspace);
UiRequestType workspace_get_active_request_type(const Workspace* workspace);
UiDialogState* workspace_get_active_dialog(Workspace* workspace);
const UiDialogState* workspace_get_active_dialog_const(const Workspace* workspace);
int workspace_document_dirty(const Workspace* workspace);
unsigned int workspace_saved_revision(const Workspace* workspace);
const char* workspace_get_current_document_path(const Workspace* workspace);

void workspace_set_status_message(Workspace* workspace, const char* message);
void workspace_set_statusf(Workspace* workspace, const char* fmt, ...);
void workspace_set_layout(Workspace* workspace, WorkspaceLayout layout);
int workspace_selection_preview_active(const Workspace* workspace);
Vec2 workspace_selection_preview_delta(const Workspace* workspace);
void workspace_mark_saved(Workspace* workspace);
void workspace_sync_document_dirty(Workspace* workspace);
void workspace_clear_clipboard(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_H */
