/**
 * @file workspace_file_commands.c
 * @brief Workspace-owned file command execution helpers.
 */
#include <app/workspace_file_commands.h>

#include <app/workspace.h>
#include <app/workspace_actions.h>
#include <app/workspace_dialogs.h>
#include <base/path_utils.h>

#include <stdio.h>

int workspace_file_new_document(Workspace* workspace)
{
    return workspace_request_action(workspace, WORKSPACE_ACTION_NEW_DOCUMENT);
}

int workspace_file_open_document(Workspace* workspace)
{
    return workspace_request_action(workspace, WORKSPACE_ACTION_OPEN_DOCUMENT);
}

int workspace_file_save_document(Workspace* workspace)
{
    return workspace_execute_service(workspace, WORKSPACE_SERVICE_SAVE_DOCUMENT);
}

int workspace_file_save_document_as(Workspace* workspace)
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
                                         path_utils_basename_or_default(path,
                                                                        "document.json"),
                                         directory);
}

int workspace_file_export_png(Workspace* workspace)
{
    return workspace_execute_service(workspace, WORKSPACE_SERVICE_EXPORT_PNG);
}

int workspace_file_exit_application(Workspace* workspace)
{
    return workspace_request_action(workspace, WORKSPACE_ACTION_EXIT_APPLICATION);
}
