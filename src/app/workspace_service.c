/**
 * @file workspace_service.c
 * @brief Document lifecycle operations for the workspace.
 *
 * Extracted from application.c to separate document manipulation
 * from the application shell and reduce the "god object" pattern.
 */
#include <app/workspace_service.h>

#include <app/extension_loader.h>
#include <base/log.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
#include <document/document.h>
#include <document/persistence.h>
#include <input/keymap.h>
#include <model/selection.h>
#include <tools/tool_controller.h>

#include <string.h>

static const char* service_default_document_path(void)
{
    return "document.json";
}

static void workspace_service_reset_runtime_state(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    selection_set_clear(&workspace->session.selection);
    workspace_clear_clipboard(workspace);
    command_executor_shutdown(&workspace->core.commands);
    command_executor_init(&workspace->core.commands);
    tool_controller_shutdown(&workspace->core.tools);
    tool_controller_init(&workspace->core.tools);
}

static void workspace_service_reset_canvas_state(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    canvas_view_set_center_zoom(&workspace->core.canvas,
                                vec2_make(0.0f, 0.0f), 1.0f);
}

int workspace_init(Workspace* workspace, RectF viewport, const char* keymap_path)
{
    if (!workspace) {
        return 0;
    }

    document_init(&workspace->core.document);

    if (!extension_loader_register_all()) {
        return 0;
    }

    if (!command_executor_init(&workspace->core.commands)) {
        return 0;
    }

    canvas_view_init(&workspace->core.canvas, &workspace->core.document, viewport);
    if (!register_builtin_tools()) {
        return 0;
    }
    tool_controller_init(&workspace->core.tools);
    keymap_init(&workspace->session.keymap, keymap_path);
    return 1;
}

void workspace_shutdown(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    tool_controller_shutdown(&workspace->core.tools);
    command_executor_shutdown(&workspace->core.commands);
    keymap_shutdown(&workspace->session.keymap);
    workspace_clear_clipboard(workspace);
    selection_set_shutdown(&workspace->session.selection);
    document_shutdown(&workspace->core.document);
}

int workspace_service_new_document(Workspace* workspace)
{
    if (!workspace) {
        return 0;
    }

    document_reset(&workspace->core.document);
    workspace_service_reset_runtime_state(workspace);
    workspace_service_reset_canvas_state(workspace);
    workspace->session.current_document_path[0] = '\0';
    workspace_mark_saved(workspace);
    workspace_set_status_message(workspace, "New empty document");
    return 1;
}

int workspace_service_save_to_path(Workspace* workspace, const char* path)
{
    if (!document_save_json(&workspace->core.document, path)) {
        LOG_ERROR("%s", "Save document failed");
        workspace_set_statusf(workspace, "Save failed: %s", path);
        return 0;
    }

    workspace_service_set_document_path(workspace, path);
    workspace_mark_saved(workspace);
    workspace_set_statusf(workspace, "Saved document: %s", path);
    LOG_INFO("Saved document: %s", path);
    return 1;
}

int workspace_service_save(Workspace* workspace)
{
    return workspace_service_save_to_path(workspace,
                                          workspace_service_document_path(workspace));
}

int workspace_service_load_from_path(Workspace* workspace, const char* path)
{
    if (!workspace_service_file_exists(path)) {
        LOG_WARN("Document file not found: %s", path);
        workspace_set_statusf(workspace, "Document not found: %s", path);
        return 0;
    }

    if (!document_load_json(&workspace->core.document, path)) {
        LOG_ERROR("%s", "Load document failed");
        workspace_set_statusf(workspace, "Load failed: %s", path);
        return 0;
    }

    workspace_service_reset_runtime_state(workspace);
    workspace_service_set_document_path(workspace, path);
    workspace_mark_saved(workspace);
    workspace_set_statusf(workspace, "Loaded document: %s", path);
    LOG_INFO("Loaded document: %s", path);
    return 1;
}

int workspace_service_load(Workspace* workspace)
{
    return workspace_service_load_from_path(workspace,
                                            workspace_service_document_path(workspace));
}

int workspace_service_file_exists(const char* path)
{
    FILE* file = NULL;

    if (!path || path[0] == '\0') {
        return 0;
    }

    file = fopen(path, "rb");
    if (!file) {
        return 0;
    }

    fclose(file);
    return 1;
}

const char* workspace_service_document_path(const Workspace* workspace)
{
    if (!workspace) {
        return service_default_document_path();
    }

    if (workspace->session.current_document_path[0] != '\0') {
        return workspace->session.current_document_path;
    }
    return service_default_document_path();
}

void workspace_service_set_document_path(Workspace* workspace, const char* path)
{
    if (!workspace || !path) {
        return;
    }

    strncpy(workspace->session.current_document_path, path,
            sizeof(workspace->session.current_document_path) - 1u);
    workspace->session.current_document_path
        [sizeof(workspace->session.current_document_path) - 1u] = '\0';
}
