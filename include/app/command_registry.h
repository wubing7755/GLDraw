/**
 * @file command_registry.h
 * @brief Stable editor command definitions and execution helpers.
 */
#ifndef GLDRAW_APP_COMMAND_REGISTRY_H
#define GLDRAW_APP_COMMAND_REGISTRY_H

#include <input/keymap.h>
#include <tools/tool.h>

struct Workspace;

typedef enum EditorCommand {
    EDITOR_COMMAND_NONE = 0,
    EDITOR_COMMAND_FILE_NEW,
    EDITOR_COMMAND_FILE_OPEN,
    EDITOR_COMMAND_FILE_SAVE,
    EDITOR_COMMAND_FILE_SAVE_AS,
    EDITOR_COMMAND_FILE_EXIT,
    EDITOR_COMMAND_EDIT_UNDO,
    EDITOR_COMMAND_EDIT_REDO,
    EDITOR_COMMAND_EDIT_CUT,
    EDITOR_COMMAND_EDIT_COPY,
    EDITOR_COMMAND_EDIT_PASTE,
    EDITOR_COMMAND_EDIT_DELETE,
    EDITOR_COMMAND_EDIT_SELECT_ALL,
    EDITOR_COMMAND_VIEW_ZOOM_IN,
    EDITOR_COMMAND_VIEW_ZOOM_OUT,
    EDITOR_COMMAND_VIEW_ZOOM_FIT,
    EDITOR_COMMAND_VIEW_TOGGLE_GRID,
    EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR,
    EDITOR_COMMAND_TOOL_SELECT,
    EDITOR_COMMAND_TOOL_PAN,
    EDITOR_COMMAND_TOOL_LINE,
    EDITOR_COMMAND_TOOL_RECT,
    EDITOR_COMMAND_TOOL_ELLIPSE,
    EDITOR_COMMAND_HELP_SHORTCUTS,
    EDITOR_COMMAND_HELP_ABOUT,
    EDITOR_COMMAND_MODAL_CONFIRM,
    EDITOR_COMMAND_MODAL_CANCEL
} EditorCommand;

typedef struct CommandDescriptor {
    EditorCommand command;
    const char* id;
    const char* label;
    KeyScope scope;
    int menu_id;
} CommandDescriptor;

/** Look up a command descriptor by stable identifier. */
const CommandDescriptor* command_registry_find_by_id(const char* command_id);
/** Look up a command descriptor by menu action ID. */
const CommandDescriptor* command_registry_find_by_menu_id(int id);
/** Check whether a command is intentionally exposed and executable in the current build/runtime. */
int command_registry_is_available(const struct Workspace* workspace,
                                  EditorCommand command);
/** Check whether a menu-backed command is available in the current build/runtime. */
int command_registry_is_menu_action_available(const struct Workspace* workspace,
                                              int id);
/** Execute one command against the current editor state. */
int command_registry_execute(struct Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command);
/** Format the effective shortcut text for one menu action. */
void command_registry_format_menu_shortcut(const struct Workspace* workspace,
                                           int id,
                                           char* buffer,
                                           size_t buffer_size);

#endif /* GLDRAW_APP_COMMAND_REGISTRY_H */
