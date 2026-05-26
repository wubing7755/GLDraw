/**
 * @file command_registry.c
 * @brief Stable command definitions and execution.
 */
#include <app/command_registry.h>

#include <app/command_availability.h>
#include <app/workspace_clipboard.h>
#include <app/workspace_dialog_commands.h>
#include <app/workspace_edit_commands.h>
#include <app/workspace_file_commands.h>
#include <app/workspace_service_types.h>
#include <app/workspace_tool_commands.h>
#include <app/workspace_view_commands.h>

typedef int (*CommandExecutionFn)(Workspace* workspace);

typedef struct CommandExecutionRoute {
    EditorCommand command;
    CommandExecutionFn execute;
} CommandExecutionRoute;

static const CommandExecutionRoute COMMAND_EXECUTION_ROUTES[] = {
    {EDITOR_COMMAND_FILE_NEW, workspace_file_new_document},
    {EDITOR_COMMAND_FILE_OPEN, workspace_file_open_document},
    {EDITOR_COMMAND_FILE_SAVE, workspace_file_save_document},
    {EDITOR_COMMAND_FILE_SAVE_AS, workspace_file_save_document_as},
    {EDITOR_COMMAND_FILE_EXPORT_PNG, workspace_file_export_png},
    {EDITOR_COMMAND_FILE_EXIT, workspace_file_exit_application},
    {EDITOR_COMMAND_EDIT_UNDO, workspace_edit_undo},
    {EDITOR_COMMAND_EDIT_REDO, workspace_edit_redo},
    {EDITOR_COMMAND_EDIT_CUT, workspace_clipboard_cut_selection},
    {EDITOR_COMMAND_EDIT_COPY, workspace_clipboard_copy_selection},
    {EDITOR_COMMAND_EDIT_PASTE, workspace_clipboard_paste},
    {EDITOR_COMMAND_EDIT_DELETE, workspace_edit_delete_selection},
    {EDITOR_COMMAND_EDIT_SELECT_ALL, workspace_edit_select_all},
    {EDITOR_COMMAND_VIEW_ZOOM_IN, workspace_view_zoom_in},
    {EDITOR_COMMAND_VIEW_ZOOM_OUT, workspace_view_zoom_out},
    {EDITOR_COMMAND_VIEW_ZOOM_FIT, workspace_view_zoom_to_fit},
    {EDITOR_COMMAND_VIEW_TOGGLE_GRID, workspace_view_toggle_grid},
    {EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR, workspace_view_toggle_inspector},
    {EDITOR_COMMAND_HELP_SHORTCUTS, workspace_dialog_command_toggle_shortcuts},
    {EDITOR_COMMAND_HELP_ABOUT, workspace_dialog_command_open_about},
    {EDITOR_COMMAND_MODAL_CONFIRM, workspace_dialog_command_confirm},
    {EDITOR_COMMAND_MODAL_CANCEL, workspace_dialog_command_cancel}
};

static CommandExecutionFn command_registry_lookup_execution(EditorCommand command)
{
    int i = 0;

    for (i = 0; i < (int)(sizeof(COMMAND_EXECUTION_ROUTES) /
                          sizeof(COMMAND_EXECUTION_ROUTES[0])); ++i) {
        if (COMMAND_EXECUTION_ROUTES[i].command == command) {
            return COMMAND_EXECUTION_ROUTES[i].execute;
        }
    }

    return NULL;
}

int command_registry_execute(Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command)
{
    CommandExecutionFn execute = NULL;

    if (!workspace) {
        return 0;
    }
    if (!command_availability_is_available(workspace, command)) {
        return 0;
    }
    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        return workspace_tool_activate_command(workspace, tool_context, command);
    }

    execute = command_registry_lookup_execution(command);
    return execute ? execute(workspace) : 0;
}
