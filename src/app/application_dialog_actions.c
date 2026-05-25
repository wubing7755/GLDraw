#include <app/application_dialog_actions.h>

#include <app/workspace_dialogs.h>
#include <app/workspace_service.h>
#include <base/path_utils.h>

#include "application_internal.h"

#include <stdio.h>

void application_dialog_actions_update_save_as_message(Application* app,
                                                       const char* error_text)
{
    char directory[GLDRAW_PATH_MAX];

    if (!app) {
        return;
    }

    if (!path_utils_dirname(workspace_service_document_path(app->workspace),
                            directory,
                            sizeof(directory))) {
        snprintf(directory, sizeof(directory), ".");
    }

    if (error_text && error_text[0] != '\0') {
        char message[1024];

        snprintf(message,
                 sizeof(message),
                 "%s\n\nEnter a new filename.\nThe file will be saved in the same directory:\n%s",
                 error_text,
                 directory);
        workspace_dialog_set_message(app->workspace, message);
        return;
    }

    {
        char message[1024];

        snprintf(message,
                 sizeof(message),
                 "Enter a new filename.\nThe file will be saved in the same directory:\n%s",
                 directory);
        workspace_dialog_set_message(app->workspace, message);
    }
}

void application_dialog_actions_report_startup_load_failure(Application* app,
                                                            const char* path)
{
    if (!app || !path) {
        return;
    }

    workspace_set_statusf(app->workspace, "Startup load failed: %s", path);
}

void application_dialog_actions_report_open_cancelled(Application* app)
{
    if (!app) {
        return;
    }

    workspace_set_status_message(app->workspace, "Open cancelled.");
}

void application_dialog_actions_report_open_picker_error(Application* app)
{
    if (!app) {
        return;
    }

    workspace_set_status_message(app->workspace,
                                 "Open failed: file picker unavailable or failed.");
}

void application_dialog_actions_report_export_cancelled(Application* app)
{
    if (!app) {
        return;
    }

    workspace_set_status_message(app->workspace, "Export PNG cancelled.");
}

void application_dialog_actions_report_export_picker_error(Application* app)
{
    if (!app) {
        return;
    }

    workspace_set_status_message(app->workspace,
                                 "Export PNG failed: save dialog unavailable or failed.");
}
