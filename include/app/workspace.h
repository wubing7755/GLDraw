/**
 * @file workspace.h
 * @brief Public workspace types and opaque workspace API.
 */
#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <app/ui_dialog_types.h>
#include <app/workspace_layout_types.h>
#include <app/workspace_service_types.h>
#include <base/types.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
#include <input/keymap.h>
#include <model/selection.h>
#include <tools/tool.h>
#include <tools/tool_controller.h>

ToolContext workspace_tool_context(Workspace* workspace);
Document* workspace_get_document(Workspace* workspace);
const Document* workspace_get_document_const(const Workspace* workspace);
CommandExecutor* workspace_get_command_executor(Workspace* workspace);
const CommandExecutor* workspace_get_command_executor_const(const Workspace* workspace);
CanvasView* workspace_get_canvas(Workspace* workspace);
const CanvasView* workspace_get_canvas_const(const Workspace* workspace);
ToolController* workspace_get_tool_controller(Workspace* workspace);
const ToolController* workspace_get_tool_controller_const(const Workspace* workspace);
EditorKeymap* workspace_get_keymap(Workspace* workspace);
const EditorKeymap* workspace_get_keymap_const(const Workspace* workspace);
SelectionSet* workspace_get_selection(Workspace* workspace);
const SelectionSet* workspace_get_selection_const(const Workspace* workspace);
const char* workspace_get_status_message(const Workspace* workspace);
WorkspaceLayout workspace_get_layout(const Workspace* workspace);
UiRequestType workspace_get_active_request_type(const Workspace* workspace);
UiDialogState* workspace_get_active_dialog(Workspace* workspace);
const UiDialogState* workspace_get_active_dialog_const(const Workspace* workspace);
int workspace_document_dirty(const Workspace* workspace);
unsigned int workspace_saved_revision(const Workspace* workspace);
const char* workspace_get_current_document_path(const Workspace* workspace);
int workspace_clipboard_count(const Workspace* workspace);
int workspace_service_available(const Workspace* workspace, WorkspaceServiceType service);
int workspace_execute_service(Workspace* workspace, WorkspaceServiceType service);

void workspace_set_status_message(Workspace* workspace, const char* message);
void workspace_set_statusf(Workspace* workspace, const char* fmt, ...);
void workspace_set_layout(Workspace* workspace, WorkspaceLayout layout);
void workspace_set_service_callbacks(Workspace* workspace,
                                     WorkspaceCommandFn save_document,
                                     WorkspaceCommandFn save_as_document,
                                     WorkspaceCommandFn export_png,
                                     WorkspaceCommandFn load_document,
                                     WorkspaceActionExecutorFn execute_action,
                                     void* command_user_data);
int workspace_selection_preview_active(const Workspace* workspace);
Vec2 workspace_selection_preview_delta(const Workspace* workspace);
void workspace_mark_saved(Workspace* workspace);
void workspace_sync_document_dirty(Workspace* workspace);
void workspace_clear_clipboard(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_H */
