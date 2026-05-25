/**
 * @file command_registry.c
 * @brief Stable command definitions and execution.
 */
#include <app/command_registry.h>

#include <app/command_availability.h>
#include <app/command_catalog.h>
#include <app/workspace.h>
#include <app/workspace_actions.h>
#include <app/workspace_clipboard.h>
#include <app/workspace_dialogs.h>
#include <app/workspace_view_commands.h>
#include <base/path_utils.h>
#include <commands/command.h>
#include <document/document.h>
#include <tools/tool_controller.h>
#include <ui/ui_menu_def.h>

#include <stdio.h>
#include <string.h>

static int command_registry_open_save_as_dialog(Workspace* workspace)
{
    char directory[GLDRAW_PATH_MAX];
    const char* path = NULL;

    if (!workspace) {
        return 0;
    }
    if (workspace_modal_is_active(workspace)) {
        return 0;
    }

    path = workspace_get_current_document_path(workspace);
    path = path && path[0] != '\0'
               ? path
               : "document.json";
    if (!path_utils_dirname(path, directory, sizeof(directory))) {
        snprintf(directory, sizeof(directory), ".");
    }
    return workspace_dialog_open_save_as(workspace,
                                         path_utils_basename_or_default(path, "document.json"),
                                         directory);
}

static void command_registry_prune_noneditable_selection(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    SelectionSet* selection = workspace_get_selection(workspace);
    int i = 0;

    if (!document || !selection) {
        return;
    }

    while (i < selection->count) {
        ObjectId id = selection->ids[i];
        if (!document_find_object(document, id) ||
            document_object_is_locked(document, id)) {
            selection_set_remove(selection, id);
            continue;
        }
        ++i;
    }
}

static void command_registry_append_shortcut_line(char* buffer,
                                                  size_t buffer_size,
                                                  const Workspace* workspace,
                                                  const char* command_id,
                                                  KeyScope scope,
                                                  const char* label)
{
    const EditorKeymap* keymap = workspace_get_keymap_const(workspace);
    char shortcut[64];

    if (!buffer || buffer_size == 0u || !keymap || !command_id || !label) {
        return;
    }

    shortcut[0] = '\0';
    keymap_format_command_shortcut(keymap,
                                   command_id,
                                   scope,
                                   shortcut,
                                   sizeof(shortcut));
    if (shortcut[0] == '\0') {
        return;
    }

    snprintf(buffer + strlen(buffer),
             buffer_size - strlen(buffer),
             "  %-16s %s\n",
             shortcut,
             label);
}

static int command_registry_toggle_shortcuts_dialog(Workspace* workspace)
{
    char content[1024];
    int i = 0;

    if (!workspace) {
        return 0;
    }

    if (workspace_active_dialog_kind(workspace) == UI_DIALOG_SHORTCUTS) {
        workspace_confirm_pending_action_cancel(workspace);
        return 1;
    }
    if (workspace_modal_is_active(workspace)) {
        return 0;
    }

    content[0] = '\0';
    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "Tools\n");
    for (i = 0; i < tool_registry_count(); ++i) {
        const ToolDescriptor* descriptor = tool_registry_at(i);
        if (!descriptor || !descriptor->command_id || !descriptor->name) {
            continue;
        }
        command_registry_append_shortcut_line(content,
                                              sizeof(content),
                                              workspace,
                                              descriptor->command_id,
                                              KEY_SCOPE_GLOBAL,
                                              descriptor->name);
    }

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nFile\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "file.new", KEY_SCOPE_GLOBAL, "New Document");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "file.open", KEY_SCOPE_GLOBAL, "Open Document");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "file.save", KEY_SCOPE_GLOBAL, "Save Document");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nEdit\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.undo", KEY_SCOPE_GLOBAL, "Undo");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.redo", KEY_SCOPE_GLOBAL, "Redo");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.cut", KEY_SCOPE_GLOBAL, "Cut");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.copy", KEY_SCOPE_GLOBAL, "Copy");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.paste", KEY_SCOPE_GLOBAL, "Paste");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.delete", KEY_SCOPE_GLOBAL, "Delete Selection");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.select_all", KEY_SCOPE_GLOBAL, "Select All");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nView\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_fit", KEY_SCOPE_GLOBAL, "Zoom to Fit");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_in", KEY_SCOPE_GLOBAL, "Zoom In");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_out", KEY_SCOPE_GLOBAL, "Zoom Out");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nHelp\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "help.shortcuts", KEY_SCOPE_GLOBAL, "Toggle This Dialog");

    return workspace_dialog_open_shortcuts(workspace, content);
}
static int command_registry_delete_selection(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    CommandExecutor* executor = workspace_get_command_executor(workspace);
    SelectionSet* selection = workspace_get_selection(workspace);
    Command* command = NULL;

    if (!document || !executor || !selection || selection->count <= 0) {
        return 0;
    }
    command_registry_prune_noneditable_selection(workspace);
    if (selection->count <= 0) {
        return 0;
    }

    command = command_create_delete_selection(document, selection);
    if (!command ||
        !command_executor_execute(executor, command, document)) {
        return 0;
    }

    selection_set_clear(selection);
    workspace_sync_document_dirty(workspace);
    return 1;
}

static void command_registry_select_all(Workspace* workspace)
{
    Document* document = workspace_get_document(workspace);
    SelectionSet* selection = workspace_get_selection(workspace);
    int i = 0;

    if (!document || !selection) {
        return;
    }

    selection_set_clear(selection);
    for (i = 0; i < document->count; ++i) {
        GraphicObject* object = document_get_object_at(document, i);
        if (object &&
            !document_layer_is_locked(document, object->layer_id)) {
            selection_set_add(selection, object->id);
        }
    }
}

static void command_registry_prune_invalid_selection(Workspace* workspace)
{
    command_registry_prune_noneditable_selection(workspace);
}

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

const CommandDescriptor* command_registry_find_by_id(const char* command_id)
{
    return command_catalog_find_by_id(command_id);
}

const CommandDescriptor* command_registry_find_by_command(EditorCommand command)
{
    return command_catalog_find_by_command(command);
}

const CommandDescriptor* command_registry_find_by_menu_id(int id)
{
    return command_catalog_find_by_menu_id(id);
}

int command_registry_is_available(const Workspace* workspace,
                                  EditorCommand command)
{
    return command_availability_is_available(workspace, command);
}

const char* command_registry_unavailable_reason(const Workspace* workspace,
                                                EditorCommand command)
{
    return command_availability_unavailable_reason(workspace, command);
}

int command_registry_is_menu_action_available(const Workspace* workspace, int id)
{
    return command_availability_is_menu_action_available(workspace, id);
}

int command_registry_execute(Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command)
{
    if (!workspace) {
        return 0;
    }
    if (!command_registry_is_available(workspace, command)) {
        return 0;
    }
    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        return command_registry_activate_tool(workspace, tool_context, command);
    }

    switch (command) {
    case EDITOR_COMMAND_FILE_NEW:
        return workspace_request_action(workspace, WORKSPACE_ACTION_NEW_DOCUMENT);
    case EDITOR_COMMAND_FILE_OPEN:
        return workspace_request_action(workspace, WORKSPACE_ACTION_OPEN_DOCUMENT);
    case EDITOR_COMMAND_FILE_SAVE:
        return workspace_execute_service(workspace, WORKSPACE_SERVICE_SAVE_DOCUMENT);
    case EDITOR_COMMAND_FILE_SAVE_AS:
        return command_registry_open_save_as_dialog(workspace);
    case EDITOR_COMMAND_FILE_EXPORT_PNG:
        return workspace_execute_service(workspace, WORKSPACE_SERVICE_EXPORT_PNG);
    case EDITOR_COMMAND_FILE_EXIT:
        return workspace_request_action(workspace, WORKSPACE_ACTION_EXIT_APPLICATION);
    case EDITOR_COMMAND_EDIT_UNDO:
    {
        Document* document = workspace_get_document(workspace);
        CommandExecutor* executor = workspace_get_command_executor(workspace);
        if (command_executor_undo(executor, document)) {
            command_registry_prune_invalid_selection(workspace);
            workspace_sync_document_dirty(workspace);
            return 1;
        }
        return 0;
    }
    case EDITOR_COMMAND_EDIT_REDO:
    {
        Document* document = workspace_get_document(workspace);
        CommandExecutor* executor = workspace_get_command_executor(workspace);
        if (command_executor_redo(executor, document)) {
            command_registry_prune_invalid_selection(workspace);
            workspace_sync_document_dirty(workspace);
            return 1;
        }
        return 0;
    }
    case EDITOR_COMMAND_EDIT_CUT:
        return workspace_clipboard_cut_selection(workspace);
    case EDITOR_COMMAND_EDIT_COPY:
        return workspace_clipboard_copy_selection(workspace);
    case EDITOR_COMMAND_EDIT_PASTE:
        return workspace_clipboard_paste(workspace);
    case EDITOR_COMMAND_EDIT_DELETE:
        return command_registry_delete_selection(workspace);
    case EDITOR_COMMAND_EDIT_SELECT_ALL:
        command_registry_select_all(workspace);
        return 1;
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
        return command_registry_toggle_shortcuts_dialog(workspace);
    case EDITOR_COMMAND_HELP_ABOUT:
        return workspace_dialog_open_info(workspace,
                                          "About GLDraw",
                                          "GLDraw\nCanvas-oriented OpenGL drawing editor.\n\nCurrent build includes core document editing, undo/redo, persistence, and themeable UI.");
    case EDITOR_COMMAND_MODAL_CONFIRM:
        return workspace_confirm_pending_action_save(workspace);
    case EDITOR_COMMAND_MODAL_CANCEL:
        workspace_confirm_pending_action_cancel(workspace);
        return 1;
    case EDITOR_COMMAND_NONE:
    default:
        return 0;
    }
}

void command_registry_format_menu_shortcut(const Workspace* workspace,
                                           int id,
                                           char* buffer,
                                           size_t buffer_size)
{
    const CommandDescriptor* descriptor = NULL;
    const EditorKeymap* keymap = workspace_get_keymap_const(workspace);

    if (!buffer || buffer_size == 0u) {
        return;
    }

    buffer[0] = '\0';
    if (!keymap) {
        return;
    }

    descriptor = command_registry_find_by_menu_id(id);
    if (!descriptor) {
        return;
    }

    keymap_format_command_shortcut(keymap,
                                   descriptor->id,
                                   descriptor->scope,
                                   buffer,
                                   buffer_size);
}
