/**
 * @file command_catalog.c
 * @brief Stable command metadata and dynamic tool command descriptors.
 */
#include <app/command_catalog.h>

#include <ui/ui_menu_def.h>

#include <string.h>

#define TOOL_DESCRIPTOR_CACHE_SIZE 4

static const CommandDescriptor g_commands[] = {
    {EDITOR_COMMAND_FILE_NEW, "file.new", "New", KEY_SCOPE_GLOBAL, MENU_ID_FILE_NEW, NULL},
    {EDITOR_COMMAND_FILE_OPEN, "file.open", "Open", KEY_SCOPE_GLOBAL, MENU_ID_FILE_OPEN, NULL},
    {EDITOR_COMMAND_FILE_SAVE, "file.save", "Save", KEY_SCOPE_GLOBAL, MENU_ID_FILE_SAVE, NULL},
    {EDITOR_COMMAND_FILE_SAVE_AS, "file.save_as", "Save As", KEY_SCOPE_GLOBAL, MENU_ID_FILE_SAVE_AS, NULL},
    {EDITOR_COMMAND_FILE_EXPORT_PNG, "file.export_png", "Export as PNG", KEY_SCOPE_GLOBAL, MENU_ID_FILE_EXPORT_PNG, NULL},
    {EDITOR_COMMAND_FILE_EXIT, "file.exit", "Exit", KEY_SCOPE_GLOBAL, MENU_ID_FILE_EXIT, NULL},
    {EDITOR_COMMAND_EDIT_UNDO, "edit.undo", "Undo", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_UNDO, NULL},
    {EDITOR_COMMAND_EDIT_REDO, "edit.redo", "Redo", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_REDO, NULL},
    {EDITOR_COMMAND_EDIT_CUT, "edit.cut", "Cut", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_CUT, NULL},
    {EDITOR_COMMAND_EDIT_COPY, "edit.copy", "Copy", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_COPY, NULL},
    {EDITOR_COMMAND_EDIT_PASTE, "edit.paste", "Paste", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_PASTE, NULL},
    {EDITOR_COMMAND_EDIT_DELETE, "edit.delete", "Delete", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_DELETE, NULL},
    {EDITOR_COMMAND_EDIT_SELECT_ALL, "edit.select_all", "Select All", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_SELECT_ALL, NULL},
    {EDITOR_COMMAND_VIEW_ZOOM_IN, "view.zoom_in", "Zoom In", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_ZOOM_IN, NULL},
    {EDITOR_COMMAND_VIEW_ZOOM_OUT, "view.zoom_out", "Zoom Out", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_ZOOM_OUT, NULL},
    {EDITOR_COMMAND_VIEW_ZOOM_FIT, "view.zoom_fit", "Zoom to Fit", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_ZOOM_FIT, NULL},
    {EDITOR_COMMAND_VIEW_TOGGLE_GRID, "view.toggle_grid", "Toggle Grid", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_TOGGLE_GRID, NULL},
    {EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR, "view.toggle_inspector", "Toggle Inspector", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_TOGGLE_INSPECTOR, NULL},
    {EDITOR_COMMAND_HELP_SHORTCUTS, "help.shortcuts", "Keyboard Shortcuts", KEY_SCOPE_GLOBAL, MENU_ID_HELP_SHORTCUTS, NULL},
    {EDITOR_COMMAND_HELP_ABOUT, "help.about", "About", KEY_SCOPE_GLOBAL, MENU_ID_HELP_ABOUT, NULL},
    {EDITOR_COMMAND_MODAL_CONFIRM, "modal.confirm", "Confirm", KEY_SCOPE_MODAL, MENU_ID_HELP, NULL},
    {EDITOR_COMMAND_MODAL_CANCEL, "modal.cancel", "Cancel", KEY_SCOPE_MODAL, MENU_ID_HELP, NULL}
};

EditorCommand command_catalog_tool_command(int tool_index)
{
    return EDITOR_COMMAND_DYNAMIC_TOOL_BASE + tool_index;
}

static const CommandDescriptor* command_catalog_make_tool_descriptor(const ToolDescriptor* tool,
                                                                    int tool_index)
{
    static CommandDescriptor cache[TOOL_DESCRIPTOR_CACHE_SIZE];
    static int cache_index = 0;
    CommandDescriptor* descriptor;

    if (!tool) {
        return NULL;
    }

    descriptor = &cache[cache_index];
    cache_index = (cache_index + 1) % TOOL_DESCRIPTOR_CACHE_SIZE;

    descriptor->command = command_catalog_tool_command(tool_index);
    descriptor->id = tool->command_id;
    descriptor->label = tool->name;
    descriptor->scope = KEY_SCOPE_GLOBAL;
    descriptor->menu_id = MENU_ID_TOOL_DYNAMIC_BASE + tool_index;
    descriptor->tool_id = tool->id;
    return descriptor;
}

const ToolDescriptor* command_catalog_tool_for_command(EditorCommand command,
                                                       int* out_tool_index)
{
    int tool_index = command - EDITOR_COMMAND_DYNAMIC_TOOL_BASE;

    if (out_tool_index) {
        *out_tool_index = -1;
    }
    if (tool_index < 0 || tool_index >= tool_registry_count()) {
        return NULL;
    }
    if (out_tool_index) {
        *out_tool_index = tool_index;
    }
    return tool_registry_at(tool_index);
}

static const CommandDescriptor* command_catalog_find_dynamic_tool_by_id(const char* command_id)
{
    int i = 0;

    if (!command_id) {
        return NULL;
    }

    for (i = 0; i < tool_registry_count(); ++i) {
        const ToolDescriptor* tool = tool_registry_at(i);
        if (tool && tool->command_id && strcmp(tool->command_id, command_id) == 0) {
            return command_catalog_make_tool_descriptor(tool, i);
        }
    }

    return NULL;
}

const CommandDescriptor* command_catalog_find_by_id(const char* command_id)
{
    size_t i = 0u;

    if (!command_id) {
        return NULL;
    }

    for (i = 0u; i < sizeof(g_commands) / sizeof(g_commands[0]); ++i) {
        if (strcmp(g_commands[i].id, command_id) == 0) {
            return &g_commands[i];
        }
    }

    return command_catalog_find_dynamic_tool_by_id(command_id);
}

const CommandDescriptor* command_catalog_find_by_command(EditorCommand command)
{
    size_t i = 0u;

    for (i = 0u; i < sizeof(g_commands) / sizeof(g_commands[0]); ++i) {
        if (g_commands[i].command == command) {
            return &g_commands[i];
        }
    }

    return command_catalog_make_tool_descriptor(
        command_catalog_tool_for_command(command, NULL),
        command - EDITOR_COMMAND_DYNAMIC_TOOL_BASE);
}

const CommandDescriptor* command_catalog_find_by_menu_id(int id)
{
    size_t i = 0u;

    for (i = 0u; i < sizeof(g_commands) / sizeof(g_commands[0]); ++i) {
        if (g_commands[i].menu_id == id) {
            return &g_commands[i];
        }
    }

    if (id >= MENU_ID_TOOL_DYNAMIC_BASE) {
        return command_catalog_make_tool_descriptor(
            tool_registry_at(id - MENU_ID_TOOL_DYNAMIC_BASE),
            id - MENU_ID_TOOL_DYNAMIC_BASE);
    }

    return NULL;
}
