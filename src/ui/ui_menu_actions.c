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
 * - Uses workspace callbacks for save/load and document/history APIs for edits.
 */
#include "ui_menu_actions.h"

#include <app/workspace.h>
#include <base/log.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <platform/window.h>

#include <GLFW/glfw3.h>

static void app_workspace_new(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    document_reset(&workspace->document);

    document_history_shutdown(&workspace->history);
    if (!document_history_init(&workspace->history)) {
        LOG_ERROR("%s", "Failed to reinitialize history");
        return;
    }

    /* Reset canvas zoom and center */
    canvas_view_set_center_zoom(&workspace->canvas, vec2_make(0.0f, 0.0f), 1.0f);

    workspace->current_document_path[0] = '\0';

    workspace->saved_revision = workspace->document.revision;
    workspace->document_dirty = 0;

    LOG_INFO("%s", "Created new document");
}

static void app_zoom_to_fit(Workspace* workspace)
{
    if (!workspace || !workspace->canvas.document) {
        return;
    }

    Document* doc = &workspace->document;
    CanvasView* canvas = &workspace->canvas;

    if (doc->count == 0) {
        /* No objects, reset to default view */
        canvas_view_set_center_zoom(canvas, vec2_make(0.0f, 0.0f), 1.0f);
        return;
    }

    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    int first = 1;

    for (int i = 0; i < doc->count; i++) {
        GraphicObject* obj = doc->objects[i];
        if (!obj) continue;

        RectF bounds = object_get_bounds(obj);
        if (first) {
            min_x = bounds.x;
            max_x = bounds.x + bounds.w;
            min_y = bounds.y;
            max_y = bounds.y + bounds.h;
            first = 0;
        } else {
            if (bounds.x < min_x) min_x = bounds.x;
            if (bounds.y < min_y) min_y = bounds.y;
            if (bounds.x + bounds.w > max_x) max_x = bounds.x + bounds.w;
            if (bounds.y + bounds.h > max_y) max_y = bounds.y + bounds.h;
        }
    }

    if (first) {
        return;
    }

    float pad = 50.0f;
    min_x -= pad;
    min_y -= pad;
    max_x += pad;
    max_y += pad;

    float content_w = max_x - min_x;
    float content_h = max_y - min_y;

    /* Calculate zoom to fit */
    RectF canvas_viewport = canvas_view_viewport(canvas);
    float zoom_x = canvas_viewport.w / content_w;
    float zoom_y = canvas_viewport.h / content_h;
    float new_zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;

    new_zoom = (new_zoom < 0.1f) ? 0.1f : (new_zoom > 12.0f) ? 12.0f : new_zoom;

    /* Set new zoom and center */
    canvas_view_set_center_zoom(canvas,
                                vec2_make((min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f),
                                new_zoom);
}

static void app_toggle_grid(Workspace* workspace)
{
    if (!workspace) {
        return;
    }
    workspace->canvas.show_grid = !workspace->canvas.show_grid;
}

static void app_toggle_inspector(Workspace* workspace)
{
    (void)workspace;
    LOG_INFO("%s", "Inspector panel toggled");
}

static void app_show_shortcuts(Workspace* workspace)
{
    (void)workspace;
    LOG_INFO("%s", "Keyboard shortcuts dialog requested");
}

static void app_show_about(Workspace* workspace)
{
    (void)workspace;
    LOG_INFO("%s", "About dialog requested");
}

static int app_save_as(Workspace* workspace)
{
    if (workspace->save_document) {
        return workspace->save_document(workspace, workspace->command_user_data);
    }
    return 0;
}

static int app_export_png(Workspace* workspace)
{
    (void)workspace;
    // TODO: Implement PNG export via framebuffer capture
    LOG_INFO("%s", "Export PNG requested");
    return 0;
}

int ui_menu_is_action_available(MenuId id)
{
    switch (id) {
    case MENU_ID_FILE_EXPORT_PNG:
    case MENU_ID_EDIT_DELETE:
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
    if (!workspace) {
        return;
    }

    switch (id) {
    /* File menu */
    case MENU_ID_FILE_NEW:
        app_workspace_new(workspace);
        break;

    case MENU_ID_FILE_OPEN:
        if (workspace->load_document) {
            workspace->load_document(workspace, workspace->command_user_data);
        }
        break;

    case MENU_ID_FILE_SAVE:
        if (workspace->save_document) {
            workspace->save_document(workspace, workspace->command_user_data);
        }
        break;

    case MENU_ID_FILE_SAVE_AS:
        app_save_as(workspace);
        break;

    case MENU_ID_FILE_EXPORT_PNG:
        app_export_png(workspace);
        break;

    case MENU_ID_FILE_EXIT:
        glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
        break;

    /* Edit menu */
    case MENU_ID_EDIT_UNDO:
        if (document_history_undo(&workspace->history, &workspace->document)) {
            workspace_sync_document_dirty(workspace);
        }
        break;

    case MENU_ID_EDIT_REDO:
        if (document_history_redo(&workspace->history, &workspace->document)) {
            workspace_sync_document_dirty(workspace);
        }
        break;

    case MENU_ID_EDIT_SELECT_ALL:
        for (int i = 0; i < workspace->document.count; i++) {
            document_selection_add(&workspace->document,
                                   workspace->document.objects[i]->id);
        }
        break;

    case MENU_ID_EDIT_DELETE:
    case MENU_ID_EDIT_CUT:
    case MENU_ID_EDIT_COPY:
    case MENU_ID_EDIT_PASTE:
        // TODO: cut/copy/paste/delete operations not yet implemented
        LOG_INFO("Menu action %d not yet implemented", id);
        break;

    /* View menu */
    case MENU_ID_VIEW_ZOOM_IN:
    {
        Vec2 center = canvas_view_center(&workspace->canvas);
        canvas_view_zoom_at_screen_point(&workspace->canvas, 1.25f,
                                         canvas_view_world_to_screen(&workspace->canvas, center));
        break;
    }

    case MENU_ID_VIEW_ZOOM_OUT:
    {
        Vec2 center = canvas_view_center(&workspace->canvas);
        canvas_view_zoom_at_screen_point(&workspace->canvas, 0.8f,
                                         canvas_view_world_to_screen(&workspace->canvas, center));
        break;
    }

    case MENU_ID_VIEW_ZOOM_FIT:
        app_zoom_to_fit(workspace);
        break;

    case MENU_ID_VIEW_TOGGLE_GRID:
        app_toggle_grid(workspace);
        break;

    case MENU_ID_VIEW_TOGGLE_INSPECTOR:
        app_toggle_inspector(workspace);
        break;

    /* Help menu */
    case MENU_ID_HELP_SHORTCUTS:
        app_show_shortcuts(workspace);
        break;

    case MENU_ID_HELP_ABOUT:
        app_show_about(workspace);
        break;

    default:
        LOG_WARN("Unknown menu action: %d", id);
        break;
    }
}
