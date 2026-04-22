/**
 * @file ui_menu_actions.c
 * @brief Menu action dispatch implementation.
 *
 * Role in project:
 * - Bridges menu IDs into stable editor commands.
 * - Keeps menu handling thin by delegating business logic to the command registry.
 */
#include "ui_menu_actions.h"

#include <app/command_registry.h>
#include <app/workspace.h>
#include <base/log.h>

static int app_export_png(Workspace* workspace)
{
    (void)workspace;
    LOG_INFO("%s", "Export PNG requested");
    return 0;
}

int ui_menu_is_action_available(MenuId id)
{
    switch (id) {
    case MENU_ID_FILE_EXPORT_PNG:
    case MENU_ID_EDIT_CUT:
    case MENU_ID_EDIT_COPY:
    case MENU_ID_EDIT_PASTE:
        return 0;
    default:
        return 1;
    }
}

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
    if (id == MENU_ID_EDIT_CUT || id == MENU_ID_EDIT_COPY || id == MENU_ID_EDIT_PASTE) {
        LOG_INFO("Menu action %d not yet implemented", id);
        return;
    }

    descriptor = command_registry_find_by_menu_id((int)id);
    if (!descriptor) {
        LOG_WARN("Unknown menu action: %d", id);
        return;
    }

    command_registry_execute(workspace, NULL, descriptor->command);
}
