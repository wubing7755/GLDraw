#include <app/application_file_actions.h>

#include <app/application_dialog_actions.h>
#include <app/workspace_dialogs.h>
#include <app/workspace_service.h>
#include <base/log.h>
#include <base/path_utils.h>
#include <platform/file_dialog.h>
#include <render/render_system.h>

#include "application_internal.h"

#include <stdio.h>
#include <string.h>

static void application_suggest_png_export_filename(const Application* app,
                                                    char* buffer,
                                                    size_t buffer_size)
{
    const char* basename = path_utils_basename_or_default(
        workspace_service_document_path(&app->workspace),
        "document.json");
    size_t length = 0u;

    if (!buffer || buffer_size == 0u) {
        return;
    }

    buffer[0] = '\0';
    length = strlen(basename);
    if (length > 5u && path_utils_has_extension(basename, ".json")) {
        length -= 5u;
    }
    if (length == 0u) {
        snprintf(buffer, buffer_size, "document.png");
        return;
    }
    if (length + 5u >= buffer_size) {
        snprintf(buffer, buffer_size, "document.png");
        return;
    }

    memcpy(buffer, basename, length);
    memcpy(buffer + length, ".png", 5u);
}

static int application_copy_png_export_path(const char* selected_path,
                                            char* output,
                                            size_t output_size)
{
    size_t path_length = 0u;

    if (!selected_path || !output || output_size == 0u || selected_path[0] == '\0') {
        return 0;
    }

    path_length = strlen(selected_path);
    if (path_utils_has_extension(selected_path, ".png")) {
        if (path_length + 1u > output_size) {
            return 0;
        }
        memcpy(output, selected_path, path_length + 1u);
        return 1;
    }

    if (path_length + 5u > output_size) {
        return 0;
    }

    memcpy(output, selected_path, path_length);
    memcpy(output + path_length, ".png", 5u);
    return 1;
}

int application_new_document(Application* app)
{
    if (!app) {
        return 0;
    }

    return workspace_service_new_document(&app->workspace);
}

int application_save_document_to_path(Application* app, const char* path)
{
    if (!app) {
        return 0;
    }

    return workspace_service_save_to_path(&app->workspace, path);
}

int application_save_document(Application* app)
{
    if (!app) {
        return 0;
    }

    return workspace_service_save(&app->workspace);
}

int application_save_as_document(Application* app)
{
    char filename[GLDRAW_PATH_MAX];
    char target_path[GLDRAW_PATH_MAX];
    const char* input = NULL;

    if (!app) {
        return 0;
    }

    input = workspace_dialog_input_text(&app->workspace);
    if (!path_utils_copy_trimmed(input, filename, sizeof(filename)) ||
        !path_utils_is_safe_filename(filename)) {
        workspace_set_status_message(&app->workspace, "Save As failed: invalid filename");
        application_dialog_actions_update_save_as_message(
            app,
            "Invalid filename. Use a simple file name without path separators or reserved characters.");
        return 0;
    }

    if (!path_utils_join_same_directory(workspace_service_document_path(&app->workspace),
                                        filename,
                                        ".json",
                                        target_path,
                                        sizeof(target_path))) {
        workspace_set_status_message(&app->workspace, "Save As failed: path too long");
        application_dialog_actions_update_save_as_message(
            app,
            "Invalid filename. The resulting path is too long.");
        return 0;
    }

    return application_save_document_to_path(app, target_path);
}

int application_load_document_from_path(Application* app, const char* path)
{
    if (!app) {
        return 0;
    }

    return workspace_service_load_from_path(&app->workspace, path);
}

int application_load_document(Application* app)
{
    if (!app) {
        return 0;
    }

    return workspace_service_load(&app->workspace);
}

int application_open_document_with_picker(Application* app)
{
    char selected_path[GLDRAW_PATH_MAX];
    PlatformFileDialogResult result;

    if (!app) {
        return 0;
    }

    result = platform_file_dialog_open_document(selected_path, sizeof(selected_path));
    if (result == PLATFORM_FILE_DIALOG_CANCELLED) {
        application_dialog_actions_report_open_cancelled(app);
        return 1;
    }
    if (result == PLATFORM_FILE_DIALOG_ERROR) {
        application_dialog_actions_report_open_picker_error(app);
        return 0;
    }

    return application_load_document_from_path(app, selected_path);
}

int application_request_export_png(Application* app)
{
    char suggested_filename[GLDRAW_PATH_MAX];
    char selected_path[GLDRAW_PATH_MAX];
    PlatformFileDialogResult result;

    if (!app) {
        return 0;
    }

    application_suggest_png_export_filename(app,
                                            suggested_filename,
                                            sizeof(suggested_filename));
    result = platform_file_dialog_save_png(selected_path,
                                           sizeof(selected_path),
                                           suggested_filename);
    if (result == PLATFORM_FILE_DIALOG_CANCELLED) {
        application_dialog_actions_report_export_cancelled(app);
        return 1;
    }
    if (result == PLATFORM_FILE_DIALOG_ERROR) {
        application_dialog_actions_report_export_picker_error(app);
        return 0;
    }

    if (!application_copy_png_export_path(selected_path,
                                          app->pending_export_png_path,
                                          sizeof(app->pending_export_png_path))) {
        workspace_set_status_message(&app->workspace, "Export PNG failed: path too long.");
        return 0;
    }

    app->pending_export_png = 1;
    workspace_set_statusf(&app->workspace,
                          "Export PNG queued: %s",
                          app->pending_export_png_path);
    return 1;
}

void application_flush_pending_export_png(Application* app)
{
    char export_path[GLDRAW_PATH_MAX];

    if (!app || !app->pending_export_png) {
        return;
    }

    snprintf(export_path, sizeof(export_path), "%s", app->pending_export_png_path);
    app->pending_export_png = 0;
    app->pending_export_png_path[0] = '\0';

    if (render_system_export_png(app->renderer,
                                 &app->workspace.core.canvas,
                                 export_path)) {
        workspace_set_statusf(&app->workspace, "Exported PNG: %s", export_path);
        LOG_INFO("Exported PNG: %s", export_path);
        return;
    }

    workspace_set_statusf(&app->workspace, "Export PNG failed: %s", export_path);
    LOG_ERROR("Export PNG failed: %s", export_path);
}

void application_open_startup_document(Application* app)
{
    const char* path = NULL;

    if (!app) {
        return;
    }

    path = workspace_service_document_path(&app->workspace);
    if (workspace_service_file_exists(path)) {
        if (!application_load_document(app)) {
            application_dialog_actions_report_startup_load_failure(app, path);
        }
        return;
    }

    workspace_set_status_message(&app->workspace, "New empty document");
}
