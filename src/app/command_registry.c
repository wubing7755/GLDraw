/**
 * @file command_registry.c
 * @brief Stable command definitions and execution.
 */
#include <app/command_registry.h>

#include <app/command_availability.h>
#include <app/command_catalog.h>
#include <app/workspace.h>
#include <app/workspace_clipboard.h>
#include <app/workspace_dialog_commands.h>
#include <app/workspace_edit_commands.h>
#include <app/workspace_file_commands.h>
#include <app/workspace_view_commands.h>
#include <tools/tool_controller.h>

static int command_registry_activate_tool(Workspace* workspace,
                                          ToolContext* tool_context,
                                          EditorCommand command)
{
    const CommandDescriptor* descriptor = command_catalog_find_by_command(command);
    ToolController* tools = workspace_get_tool_controller(workspace);

    if (!tools || !tool_context || !descriptor || !descriptor->tool_id) {
        return 0;
    }

    return tool_controller_set_active(tools, tool_context, descriptor->tool_id);
}

int command_registry_execute(Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command)
{
    if (!workspace) {
        return 0;
    }
    if (!command_availability_is_available(workspace, command)) {
        return 0;
    }
    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        return command_registry_activate_tool(workspace, tool_context, command);
    }

    switch (command) {
    case EDITOR_COMMAND_FILE_NEW:
        return workspace_file_new_document(workspace);
    case EDITOR_COMMAND_FILE_OPEN:
        return workspace_file_open_document(workspace);
    case EDITOR_COMMAND_FILE_SAVE:
        return workspace_file_save_document(workspace);
    case EDITOR_COMMAND_FILE_SAVE_AS:
        return workspace_file_save_document_as(workspace);
    case EDITOR_COMMAND_FILE_EXPORT_PNG:
        return workspace_file_export_png(workspace);
    case EDITOR_COMMAND_FILE_EXIT:
        return workspace_file_exit_application(workspace);
    case EDITOR_COMMAND_EDIT_UNDO:
        return workspace_edit_undo(workspace);
    case EDITOR_COMMAND_EDIT_REDO:
        return workspace_edit_redo(workspace);
    case EDITOR_COMMAND_EDIT_CUT:
        return workspace_clipboard_cut_selection(workspace);
    case EDITOR_COMMAND_EDIT_COPY:
        return workspace_clipboard_copy_selection(workspace);
    case EDITOR_COMMAND_EDIT_PASTE:
        return workspace_clipboard_paste(workspace);
    case EDITOR_COMMAND_EDIT_DELETE:
        return workspace_edit_delete_selection(workspace);
    case EDITOR_COMMAND_EDIT_SELECT_ALL:
        return workspace_edit_select_all(workspace);
    case EDITOR_COMMAND_VIEW_ZOOM_IN:
        return workspace_view_zoom_in(workspace);
    case EDITOR_COMMAND_VIEW_ZOOM_OUT:
        return workspace_view_zoom_out(workspace);
    case EDITOR_COMMAND_VIEW_ZOOM_FIT:
        return workspace_view_zoom_to_fit(workspace);
    case EDITOR_COMMAND_VIEW_TOGGLE_GRID:
        return workspace_view_toggle_grid(workspace);
    case EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR:
        return 1;
    case EDITOR_COMMAND_HELP_SHORTCUTS:
        return workspace_dialog_command_toggle_shortcuts(workspace);
    case EDITOR_COMMAND_HELP_ABOUT:
        return workspace_dialog_command_open_about(workspace);
    case EDITOR_COMMAND_MODAL_CONFIRM:
        return workspace_dialog_command_confirm(workspace);
    case EDITOR_COMMAND_MODAL_CANCEL:
        return workspace_dialog_command_cancel(workspace);
    case EDITOR_COMMAND_NONE:
    default:
        return 0;
    }
}
