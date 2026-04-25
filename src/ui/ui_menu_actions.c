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
#include <app/workspace.h>
#include <base/log.h>

/**
 * @brief Executes PNG export command (not yet implemented).
 * @param workspace [in,out] Workspace instance (currently unused).
 * @return Currently always returns 0.
 */
static int app_export_png(Workspace* workspace)
{
    (void)workspace;
    LOG_INFO("%s", "Export PNG requested");
    return 0;
}

/**
 * @brief Checks if menu action is currently available.
 * @param id [in] Menu action ID.
 * @return Non-zero if available, 0 if unavailable.
 */
int ui_menu_is_action_available(MenuId id)
{
    switch (id) {
    case MENU_ID_FILE_EXPORT_PNG:
        return 0;
    default:
        return 1;
    }
}

/**
 * @brief Executes menu action dispatch.
 * @param workspace [in,out] Workspace instance.
 * @param id [in] Menu action ID.
 * @return None.
 */
void ui_menu_execute(Workspace* workspace, MenuId id)
{
    const CommandDescriptor* descriptor = NULL;

    if (!workspace) {
        return;
    }

    if (id == MENU_ID_FILE_EXPORT_PNG) {
        app_export_png(workspace);
        return;
    }

    descriptor = command_registry_find_by_menu_id((int)id);
    if (!descriptor) {
        LOG_WARN("Unknown menu action: %d", id);
        return;
    }

    command_registry_execute(workspace, NULL, descriptor->command);
}
