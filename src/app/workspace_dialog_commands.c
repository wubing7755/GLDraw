/**
 * @file workspace_dialog_commands.c
 * @brief Workspace-owned help and modal dialog command helpers.
 */
#include <app/workspace_dialog_commands.h>

#include <app/workspace_actions.h>
#include <app/workspace_dialogs.h>

#include <stdio.h>
#include <string.h>

static void workspace_dialog_command_append_shortcut_line(char* buffer,
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

int workspace_dialog_command_toggle_shortcuts(Workspace* workspace)
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
        workspace_dialog_command_append_shortcut_line(content,
                                                      sizeof(content),
                                                      workspace,
                                                      descriptor->command_id,
                                                      KEY_SCOPE_GLOBAL,
                                                      descriptor->name);
    }

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nFile\n");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "file.new", KEY_SCOPE_GLOBAL, "New Document");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "file.open", KEY_SCOPE_GLOBAL, "Open Document");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "file.save", KEY_SCOPE_GLOBAL, "Save Document");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nEdit\n");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "edit.undo", KEY_SCOPE_GLOBAL, "Undo");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "edit.redo", KEY_SCOPE_GLOBAL, "Redo");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "edit.cut", KEY_SCOPE_GLOBAL, "Cut");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "edit.copy", KEY_SCOPE_GLOBAL, "Copy");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "edit.paste", KEY_SCOPE_GLOBAL, "Paste");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "edit.delete", KEY_SCOPE_GLOBAL, "Delete Selection");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "edit.select_all", KEY_SCOPE_GLOBAL, "Select All");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nView\n");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_fit", KEY_SCOPE_GLOBAL, "Zoom to Fit");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_in", KEY_SCOPE_GLOBAL, "Zoom In");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_out", KEY_SCOPE_GLOBAL, "Zoom Out");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nHelp\n");
    workspace_dialog_command_append_shortcut_line(content, sizeof(content), workspace, "help.shortcuts", KEY_SCOPE_GLOBAL, "Toggle This Dialog");

    return workspace_dialog_open_shortcuts(workspace, content);
}

int workspace_dialog_command_open_about(Workspace* workspace)
{
    return workspace_dialog_open_info(
        workspace,
        "About GLDraw",
        "GLDraw\nCanvas-oriented OpenGL drawing editor.\n\nCurrent build includes core document editing, undo/redo, persistence, and themeable UI.");
}

int workspace_dialog_command_confirm(Workspace* workspace)
{
    return workspace_confirm_pending_action_save(workspace);
}

int workspace_dialog_command_cancel(Workspace* workspace)
{
    workspace_confirm_pending_action_cancel(workspace);
    return 1;
}
