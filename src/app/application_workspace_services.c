#include "application_internal.h"

#include <app/application_file_actions.h>
#include <app/workspace.h>

static int app_exit_application(Application* app)
{
    if (!app) {
        return 0;
    }

    platform_window_set_should_close(&app->window, 1);
    workspace_set_status_message(app->workspace, "Closing application");
    return 1;
}

static int app_workspace_save(Workspace* workspace, void* user_data)
{
    (void)workspace;
    return application_save_document((Application*)user_data);
}

static int app_workspace_save_as(Workspace* workspace, void* user_data)
{
    (void)workspace;
    return application_save_as_document((Application*)user_data);
}

static int app_workspace_export_png(Workspace* workspace, void* user_data)
{
    (void)workspace;
    return application_request_export_png((Application*)user_data);
}

static int app_workspace_load(Workspace* workspace, void* user_data)
{
    (void)workspace;
    return application_load_document((Application*)user_data);
}

static int app_workspace_execute_action(Workspace* workspace,
                                        WorkspaceActionType action,
                                        void* user_data)
{
    Application* app = (Application*)user_data;
    (void)workspace;

    switch (action) {
    case WORKSPACE_ACTION_NEW_DOCUMENT:
        return application_new_document(app);
    case WORKSPACE_ACTION_OPEN_DOCUMENT:
        return application_open_document_with_picker(app);
    case WORKSPACE_ACTION_EXIT_APPLICATION:
        return app_exit_application(app);
    case WORKSPACE_ACTION_NONE:
    default:
        return 0;
    }
}

void application_register_workspace_services(Application* app)
{
    if (!app || !app->workspace) {
        return;
    }

    workspace_set_service_callbacks(app->workspace,
                                    app_workspace_save,
                                    app_workspace_save_as,
                                    app_workspace_export_png,
                                    app_workspace_load,
                                    app_workspace_execute_action,
                                    app);
}
