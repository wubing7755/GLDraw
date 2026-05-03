/**
 * @file ui_menu_actions.c
 * @brief Menu action dispatch implementation.
 *
 * Role in project:
 * - Maps menu IDs to workspace/document/canvas operations.
 * - Centralizes shortcut-equivalent command behavior.
 *
 * Module relationships:
 * - Called by menu bar and application shortcut handling.
 * - Uses command registry to dispatch workspace commands.
 */
#include "ui_menu_actions.h"

#include <app/command_registry.h>
#include <base/log.h>

/**
 * @brief Checks if menu action is currently available.
 * @param view_model [in] View model used for command-backed items.
 * @param id [in] Menu action ID.
 * @return Non-zero if available, 0 if unavailable.
 */
int ui_menu_is_action_available(const EditorViewModel* view_model, MenuId id)
{
    const CommandDescriptor* descriptor = NULL;

    switch (id) {
    case MENU_ID_FILE_RECENT:
        return 0;
    default:
        descriptor = command_registry_find_by_menu_id((int)id);
        if (!descriptor) {
            return 0;
        }
        return editor_viewmodel_command_available(view_model, descriptor->command);
    }
}

/**
 * @brief Emits menu action dispatch.
 * @param sink [in] Action sink.
 * @param id [in] Menu action ID.
 * @return None.
 */
void ui_menu_execute(const EditorActionSink* sink, MenuId id)
{
    const CommandDescriptor* descriptor = NULL;
    EditorAction action;

    if (!sink) {
        return;
    }

    descriptor = command_registry_find_by_menu_id((int)id);
    if (!descriptor) {
        LOG_WARN("Unknown menu action: %d", id);
        return;
    }

    action = editor_action_make_execute_command(descriptor->command);
    editor_action_emit(sink, &action);
}
